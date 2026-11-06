#!/usr/bin/env python3
"""Test PicoNIX mcpd from Linux using the official MCP Python SDK v2.

This is a direct MCP client: no LLM is involved.
"""

import argparse
import asyncio
import sys
import traceback


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "url", nargs="?", default="http://192.168.4.1:8080/mcp",
        help="PicoNIX MCP endpoint (default: %(default)s)",
    )
    parser.add_argument(
        "--tool", choices=("get_temperature", "get_wifi_info", "all", "none"),
        default="all", help="Tool(s) to call after listing (default: %(default)s)",
    )
    parser.add_argument(
        "--mode", choices=("auto", "2026-07-28"), default="auto",
        help="MCP protocol mode (default: %(default)s)",
    )
    return parser.parse_args()


async def run(url, mode, tool_selection):
    # Import here to show an actionable dependency error instead of a traceback.
    try:
        from mcp import Client
    except ImportError as exc:
        raise RuntimeError('MCP SDK v2 is required: pip install "mcp>=2,<3"') from exc

    async with Client(url, mode=mode) as client:
        print("Protocol:", client.protocol_version)
        print("Request: tools/list", flush=True)
        result = await client.list_tools()
        names = {tool.name for tool in result.tools}
        print("Available tools:")
        for tool in result.tools:
            print(f"  {tool.name}: {tool.description or ''}")

        if tool_selection == "none":
            return
        wanted = ("get_temperature", "get_wifi_info") if tool_selection == "all" else (tool_selection,)
        failed = False
        for name in wanted:
            if name not in names:
                print(f"Tool unavailable: {name}", file=sys.stderr)
                failed = True
                continue
            print(f"\n{name}:")
            print(f"Request: tools/call ({name})", flush=True)
            answer = await client.call_tool(name, {})
            for item in answer.content:
                if item.type == "text":
                    print(item.text)
                else:
                    print(f"[{item.type} content]")
            if answer.is_error:
                print(f"Tool returned an error: {name}", file=sys.stderr)
                failed = True
        if failed:
            raise RuntimeError("Some MCP tool calls failed")


def main():
    args = parse_args()
    if not args.url.startswith("http://"):
        sys.exit("This experimental sample expects an http:// URL on an isolated LAN")
    try:
        asyncio.run(run(args.url, args.mode, args.tool))
    except (Exception, KeyboardInterrupt) as exc:
        print("MCP client failed; detailed traceback:", file=sys.stderr)
        traceback.print_exception(exc)
        sys.exit(1)


if __name__ == "__main__":
    main()
