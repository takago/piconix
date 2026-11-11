#!/usr/bin/env python3
"""PicoNIX MCP PTY test: render an 80x24 screen without a local ANSI emulator."""
import argparse
import asyncio


async def main():
    """常駐PTYのシェルでコマンドを起動し，画面取得とキー入力を実機検証する．"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", nargs="?", default="http://192.168.4.1:8080/mcp")
    parser.add_argument("--command", default="fd", help="PicoNIX app and optional argument")
    parser.add_argument("--keys", default="", help="Comma-separated key names (e.g. DOWN,ENTER)")
    args = parser.parse_args()
    try:
        from mcp import Client
    except ImportError as exc:
        raise SystemExit('Install MCP SDK v2: pip install "mcp>=2,<3"') from exc

    async with Client(args.url, mode="2026-07-28") as client:
        # 指定されたMCPツールを呼び出し，テキスト応答を返す．
        async def call(name, arguments):
            response = await client.call_tool(name, arguments)
            lines = [item.text for item in response.content if item.type == "text"]
            result = "\n".join(lines)
            if response.is_error:
                raise RuntimeError(f"{name}: {result}")
            return result

        print("initial shell screen:\n" + await call("terminal_read", {}))
        if args.command:
            print("terminal_send:", await call("terminal_send", {"text": args.command}))
            print("terminal_send:", await call("terminal_send", {"key": "ENTER"}))
        try:
            for i in range(8):
                screen = await call("terminal_read", {})
                if screen != "unchanged":
                    print("\n" + screen)
                    break
                await asyncio.sleep(0.05)
            for key in filter(None, (x.strip() for x in args.keys.split(","))):
                print("terminal_send:", key, await call("terminal_send", {"key": key}))
                await asyncio.sleep(0.08)
                print("terminal_read:\n" + await call("terminal_read", {}))
        finally:
            print("PTY remains open; exit the application with its own quit command")


if __name__ == "__main__":
    asyncio.run(main())
