#!/usr/bin/env python3
"""Chat with a local LLM and access PicoNIX through MCP (non-streaming)."""

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

# PicoNIX Pico W access point (unchanged).
MCP_URL = "http://192.168.4.1:8080/mcp"

llm = AsyncOpenAI(base_url=LLM_URL, api_key=LLM_API_KEY, timeout=120.0)

# Keep redirected output and terminals without ANSI support readable.
USE_COLOR = sys.stdout.isatty() and "NO_COLOR" not in os.environ and os.environ.get("TERM") != "dumb"
COLORS = {
    "user": "\033[36m",      # cyan: interactive prompt
    "llm": "\033[32m",       # green: assistant reply
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
                "LED表示を求められた場合はdisplay_messageを使ってください．"
                "display_messageのUDP送信成功は，LEDへの配送や表示完了を保証しません．"
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
                response = await llm.chat.completions.create(
                    model=LLM_MODEL,
                    messages=messages,
                    tools=tools or None,
                    tool_choice="auto" if tools else "none",
                )
                message = response.choices[0].message
                messages.append(message.model_dump(exclude_none=True))

                if not message.tool_calls:
                    print()
                    show("LLM >", message.content or "", "llm")
                    break

                for call in message.tool_calls:
                    name = call.function.name
                    failed = False
                    try:
                        args = json.loads(call.function.arguments or "{}")
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
                        "tool_call_id": call.id,
                        "content": output,
                    })


if __name__ == "__main__":
    asyncio.run(main())
