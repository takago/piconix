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
SHOW_THINKING = False  # True: display thinking / False: hide thinking

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
    """Show a labeled message, coloring tool and status lines when supported."""
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
    thinking_chars = 0
    progress_width = 0

    def clear_progress():
        """Remove the in-place indicator before printing answer text."""
        nonlocal progress_width
        if progress_width and sys.stdout.isatty():
            print("\r" + " " * progress_width + "\r", end="", flush=True)
        progress_width = 0

    async for chunk in stream:
        if not chunk.choices:
            continue  # e.g. final usage-only chunk
        delta = chunk.choices[0].delta
        thought = reasoning_from_delta(delta)
        reply = getattr(delta, "content", None) or ""
        for kind, fragment in (("think", thought), ("llm", reply)):
            if not fragment:
                continue
            if kind == "think" and not SHOW_THINKING:
                if active not in (None, "progress"):
                    print(flush=True)
                thinking_chars += len(fragment)
                indicator = f"...thinking ({thinking_chars})"
                if sys.stdout.isatty():
                    print("\r" + indicator.ljust(progress_width), end="", flush=True)
                    progress_width = len(indicator)
                elif active != "progress":
                    # Keep redirected logs free of repeated carriage returns.
                    print("...thinking", flush=True)
                active = "progress"
                continue
            if kind != active:
                if active == "progress":
                    clear_progress()
                elif active is not None:
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

    if active == "progress":
        clear_progress()
        if not sys.stdout.isatty():
            print(f"...thinking ({thinking_chars} chars received)", flush=True)
    elif active is not None:
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
    connected = False
    try:
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
            connected = True
    
            messages = []  # No system prompt; use MCP tools and manuals as needed.
    
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
    except Exception as exc:
        if not connected:
            show("[ERROR]", "MCPサーバに接続できません．PicoNIXでmcpdは起動していますか？", "error")
            show("[DETAIL]", str(exc), "error")
        else:
            show("[ERROR]", f"MCP通信でエラーが発生しました: {exc}", "error")
        return False
    return True


if __name__ == "__main__":
    raise SystemExit(0 if asyncio.run(main()) else 1)
