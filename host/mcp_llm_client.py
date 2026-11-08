#!/usr/bin/env python3
"""Stream a local LLM's replies, reasoning, and MCP tool calls."""

import asyncio
import json
import os
import sys

from mcp import Client
from openai import AsyncOpenAI

# Replace these placeholders with your own OpenAI-compatible server settings.
LLM_URL = "http://######:11434/v1"
LLM_MODEL = "######"
LLM_API_KEY = "######"
LLM_REASONING_EFFORT = "low"  # Ollama: none / low / medium / high

# PicoNIX Pico W access point (unchanged).
MCP_URL = "http://192.168.4.1:8080/mcp"

llm = AsyncOpenAI(base_url=LLM_URL, api_key=LLM_API_KEY, timeout=120.0)

# Keep redirected output and terminals without ANSI support readable.
USE_COLOR = sys.stdout.isatty() and "NO_COLOR" not in os.environ and os.environ.get("TERM") != "dumb"
COLORS = {
    "user": "\033[36m",      # cyan: interactive prompt
    "llm": "\033[32m",       # green: assistant reply
    "think": "\033[90m",    # gray: reasoning text
    "tool": "\033[33m",      # yellow: tool invocation
    "result": "\033[34m",    # blue: tool response
    "info": "\033[2m",       # dim: connection information
    "error": "\033[31m",     # red: errors
}


def styled(text, kind):
    """Colorize labels only; never modify message content sent to the LLM."""
    if not USE_COLOR:
        return text
    return f"{COLORS[kind]}{text}\033[0m"


def show(label, value, kind):
    line = f"{label} {value}"
    # System activity is colored as a whole; conversational text stays plain.
    print(styled(line, kind) if kind in ("tool", "result", "error", "info")
          else f"{styled(label, kind)} {value}")


def reasoning_from_delta(delta):
    """Ollama can expose thinking as reasoning_content or reasoning."""
    for key in ("reasoning_content", "reasoning"):
        value = getattr(delta, key, None)
        if value is None:
            value = (getattr(delta, "model_extra", None) or {}).get(key)
        if isinstance(value, str) and value:
            return value
    return ""


async def stream_reply(messages, tools):
    """Print streamed reasoning/content and reassemble fragmented tool calls."""
    stream = await llm.chat.completions.create(
        model=LLM_MODEL,
        messages=messages,
        tools=tools or None,
        tool_choice="auto" if tools else "none",
        reasoning_effort=LLM_REASONING_EFFORT,
        stream=True,
    )
    content = []
    tool_parts = {}
    active = None

    async for chunk in stream:
        if not chunk.choices:
            continue  # e.g. final usage-only chunk
        delta = chunk.choices[0].delta
        thought = reasoning_from_delta(delta)
        reply = getattr(delta, "content", None) or ""
        for kind, fragment in (("think", thought), ("llm", reply)):
            if not fragment:
                continue
            if kind != active:
                if active is not None:
                    print(flush=True)
                print(styled("[THINK] " if kind == "think" else "LLM > ", kind),
                      end="", flush=True)
                active = kind
            # Color the reasoning itself too, but not the assistant's answer.
            print(styled(fragment, kind) if kind == "think" else fragment,
                  end="", flush=True)
            if kind == "llm":
                content.append(fragment)

        for call in (getattr(delta, "tool_calls", None) or []):
            idx = call.index
            state = tool_parts.setdefault(idx, {"id": "", "name": "", "arguments": ""})
            if call.id:
                state["id"] = call.id
            if call.function:
                if call.function.name:
                    state["name"] += call.function.name
                if call.function.arguments:
                    state["arguments"] += call.function.arguments

    if active is not None:
        print(flush=True)
    calls = [{"id": part["id"], "type": "function",
              "function": {"name": part["name"],
                           "arguments": part["arguments"] or "{}"}}
             for _, part in sorted(tool_parts.items())]
    message = {"role": "assistant", "content": "".join(content)}
    if calls:
        message["tool_calls"] = calls
    return message, calls


async def main():
    """MCPクライアントとLLMの対話を実行する．"""
    async with Client(MCP_URL, mode="auto") as mcp:
        show("[MCP] protocol:", mcp.protocol_version, "info")
        available = (await mcp.list_tools()).tools
        tools = [{
            "type": "function",
            "function": {
                "name": t.name,
                "description": t.description or "",
                "parameters": t.input_schema,
            },
        } for t in available]
        show("[MCP] available tools:", ", ".join(t.name for t in available) or "(none)", "info")

        messages = [{
            "role": "system",
            "content": (
                "あなたは親切なAIアシスタントです．"
                "必要に応じてツールを利用して回答してください．"
                "ツールで確認していない測定値や状態は推測しないでください．"
                "複数のツールが必要なら，先に情報を取得し，結果を確認してから次のツールを呼び出してください．"
                "PicoNIXはLinuxではありません．MCPツール一覧にget_manualがあれば，"
                "PicoNIX操作の前に引数なしで一度呼び出し，内蔵マニュアル全体を確認してください．"
                "詳しい使い方はexecute_commandからhelpで調べてください．"
                "execute_commandでは非対話型コマンドだけを使用してください．"
                "mem，watch，vi，edit，fd，pongなどの全画面アプリは起動しないでください．"
                "ネットワーク情報はipやcat /proc/net/wifiを利用できます．"
                "LED表示は標準入力を利用するecho HELLO | scrollが基本です．"
                "6プロセスの制限に留意し，必要のない常駐サービスは起動しないでください．"
            ),
        }]

        while True:
            try:
                question = input("\n" + styled("User >", "user") + " ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break
            if question.lower() in ("exit", "quit"):
                break
            if not question:
                continue

            messages.append({"role": "user", "content": question})
            # Keep calling the LLM until it produces a final answer.
            while True:
                message, calls = await stream_reply(messages, tools)
                messages.append(message)

                if not calls:
                    if not message["content"]:
                        print(styled("[INFO] No response text from model", "info"))
                    break

                for call in calls:
                    name = call["function"]["name"]
                    failed = False
                    try:
                        args = json.loads(call["function"]["arguments"] or "{}")
                        if not isinstance(args, dict):
                            raise ValueError("Tool arguments must be an object")
                        print()
                        show("[TOOL]", f"{name}({json.dumps(args, ensure_ascii=False)})", "tool")
                        result = await mcp.call_tool(name, arguments=args)
                        output = "\n".join(
                            item.text for item in result.content
                            if item.type == "text"
                        )
                        if result.is_error:
                            failed = True
                            output = "Tool error: " + output
                    except Exception as exc:
                        failed = True
                        output = f"Tool error: {exc}"
                    show("[ERROR]" if failed else "[RESULT]", output,
                         "error" if failed else "result")
                    messages.append({
                        "role": "tool",
                        "tool_call_id": call["id"],
                        "content": output,
                    })


if __name__ == "__main__":
    asyncio.run(main())
