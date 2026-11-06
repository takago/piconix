#!/usr/bin/env python3
"""Chat with a local LLM and access PicoNIX through MCP (non-streaming)."""

import asyncio
import json

from mcp import Client
from openai import AsyncOpenAI

# Replace these placeholders with your own OpenAI-compatible server settings.
LLM_URL = "http://######:11434/v1"
LLM_MODEL = "######"
LLM_API_KEY = "######"

# PicoNIX Pico W access point (unchanged).
MCP_URL = "http://192.168.4.1:8080/mcp"

llm = AsyncOpenAI(base_url=LLM_URL, api_key=LLM_API_KEY, timeout=120.0)


async def main():
    async with Client(MCP_URL, mode="auto") as mcp:
        print("MCP protocol:", mcp.protocol_version)
        available = (await mcp.list_tools()).tools
        tools = [{
            "type": "function",
            "function": {
                "name": t.name,
                "description": t.description or "",
                "parameters": t.input_schema,
            },
        } for t in available]
        print("Available tools:", ", ".join(t.name for t in available))

        messages = [{
            "role": "system",
            "content": (
                "あなたは親切なAIアシスタントです．"
                "必要に応じてツールを利用して回答してください．"
                "ツールで確認していない測定値や状態は推測しないでください．"
            ),
        }]

        while True:
            try:
                question = input("\nUser > ").strip()
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
                    print("\nLLM >", message.content)
                    break

                for call in message.tool_calls:
                    name = call.function.name
                    try:
                        args = json.loads(call.function.arguments or "{}")
                        if not isinstance(args, dict):
                            raise ValueError("Tool arguments must be an object")
                        result = await mcp.call_tool(name, arguments=args)
                        output = "\n".join(
                            item.text for item in result.content
                            if item.type == "text"
                        )
                        if result.is_error:
                            output = "Tool error: " + output
                    except Exception as exc:
                        output = f"Tool error: {exc}"
                    print(f"Tool: {name}\nResult: {output}")
                    messages.append({
                        "role": "tool",
                        "tool_call_id": call.id,
                        "content": output,
                    })


if __name__ == "__main__":
    asyncio.run(main())
