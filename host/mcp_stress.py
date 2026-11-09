#!/usr/bin/env python3
"""Check repeated PicoNIX MCP HTTP requests without an LLM or automatic retries.

Each call uses a new TCP connection, matching mcpd's Connection: close behavior.
The entire body is read so an absent HTTP chunk terminator is detected.
"""
import argparse
import http.client
import json
import time
from urllib.parse import urlsplit


def send_request(url, seq, tool, command, timeout):
    """One TCP transaction; never retry an ambiguous tool execution."""
    parsed = urlsplit(url)
    if parsed.scheme != "http" or not parsed.hostname or parsed.path != "/mcp":
        raise ValueError("URL must be http://HOST:PORT/mcp")
    if parsed.username or parsed.password or parsed.query or parsed.fragment:
        raise ValueError("Credentials and URL query parameters are not supported")
    method = "tools/list" if tool == "tools/list" else "tools/call"
    meta = {
        "io.modelcontextprotocol/protocolVersion": "2026-07-28",
        "io.modelcontextprotocol/clientInfo": {"name": "mcp-stress", "version": "1"},
        "io.modelcontextprotocol/clientCapabilities": {},
    }
    params = {"_meta": meta}
    if method == "tools/call":
        params["name"] = tool
        params["arguments"] = {"command": command} if tool == "execute_command" else {}
    body = json.dumps({"jsonrpc": "2.0", "id": seq, "method": method, "params": params},
                      separators=(",", ":")).encode("ascii")
    headers = {
        "Content-Type": "application/json",
        "Accept": "application/json, text/event-stream",
        "MCP-Protocol-Version": "2026-07-28",
        "Mcp-Method": method,
        "Connection": "close",
    }
    if method == "tools/call":
        headers["Mcp-Name"] = tool
    client = http.client.HTTPConnection(parsed.hostname, parsed.port or 80, timeout=timeout)
    try:
        client.request("POST", parsed.path, body=body, headers=headers)
        response = client.getresponse()
        # HTTPConnection.read() validates the terminating 0-chunk; do not stop at HTTP 200.
        payload = response.read()
        if response.status != 200:
            raise RuntimeError(f"HTTP {response.status}: {payload[:160]!r}")
        result = json.loads(payload)
        if result.get("id") != seq or "result" not in result:
            raise ValueError(f"Unexpected JSON-RPC response: {payload[:200]!r}")
        return len(payload), response.getheader("Transfer-Encoding", "")
    finally:
        client.close()


def main():
    """Run a bounded sequence of distinct HTTP transactions and summarize failures."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", nargs="?", default="http://192.168.4.1:8080/mcp")
    parser.add_argument("--count", type=int, default=50)
    parser.add_argument("--interval-ms", type=int, default=200)
    parser.add_argument("--timeout", type=float, default=10)
    parser.add_argument("--tool", choices=("tools/list", "get_temperature", "execute_command"),
                        default="get_temperature")
    parser.add_argument("--command", default="trace run hello")
    parser.add_argument("--continue-on-error", action="store_true",
                        help="Continue with subsequent requests (never retry a failed request)")
    args = parser.parse_args()
    if not 1 <= args.count <= 10000 or args.interval_ms < 0 or args.timeout <= 0:
        parser.error("count must be 1..10000, interval nonnegative, timeout positive")
    if args.tool == "execute_command" and (not args.command or len(args.command) > 119 or
                                              any(ord(c) < 32 or ord(c) > 126 for c in args.command)):
        parser.error("command must be printable ASCII, 1..119 bytes")
    ok = 0
    for i in range(1, args.count + 1):
        started = time.monotonic()
        try:
            n, encoding = send_request(args.url, i, args.tool, args.command, args.timeout)
            ms = (time.monotonic() - started) * 1000
            print(f"{i:4}/{args.count} OK  {ms:7.1f} ms  {n} bytes  "
                  f"transfer={encoding or 'Content-Length'}", flush=True)
            ok += 1
        except (OSError, EOFError, ValueError, RuntimeError, json.JSONDecodeError,
                http.client.HTTPException) as exc:
            ms = (time.monotonic() - started) * 1000
            print(f"{i:4}/{args.count} FAIL {ms:7.1f} ms  {type(exc).__name__}: {exc}",
                  flush=True)
            if not args.continue_on_error:
                break
        if i < args.count and args.interval_ms:
            time.sleep(args.interval_ms / 1000)
    print(f"Completed: {ok} successful / {i} attempted; "
          "no failed request was automatically retried.")
    return 0 if ok == args.count else 1


if __name__ == "__main__":
    raise SystemExit(main())
