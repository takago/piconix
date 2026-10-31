#!/usr/bin/env python3
"""Minimal UDP device-file client for PicoNIX Pico W (no retries)."""
import argparse
import socket
import sys


def request(host: str, port: int, message: bytes, timeout: float) -> bytes:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(timeout)
        sock.connect((host, port))
        sock.send(message)
        response = sock.recv(256)
    if response.startswith(b"ERR"):
        raise RuntimeError("PicoNIX rejected the request")
    if not response.startswith(b"OK\n"):
        raise RuntimeError("invalid PicoNIX response")
    return response[3:]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="192.168.4.1")
    parser.add_argument("--port", type=int, default=2000)
    parser.add_argument("--timeout", type=float, default=1.0)
    parser.add_argument("command", choices=("read", "write", "enable", "disable"))
    parser.add_argument("path")
    parser.add_argument("value", nargs="?")
    parser.add_argument("--hex", action="store_true", help="hexadecimal binary payload / read output")
    args = parser.parse_args()
    if args.command == "write":
        if args.value is None:
            parser.error("write requires a value")
        try:
            payload = bytes.fromhex(args.value) if args.hex else args.value.encode("ascii")
        except (ValueError, UnicodeEncodeError) as exc:
            parser.error(str(exc))
        message = b"WRITE " + args.path.encode("ascii") + b"\n" + payload
    else:
        if args.value is not None:
            parser.error("unexpected value")
        message = args.command.upper().encode("ascii") + b" " + args.path.encode("ascii")
    if len(message) > 128:
        parser.error("request exceeds 128 bytes")
    try:
        result = request(args.host, args.port, message, args.timeout)
    except (OSError, RuntimeError) as exc:
        print(f"piconixctl: {exc}", file=sys.stderr)
        return 1
    if args.command == "read":
        if args.hex or args.path == "/dev/gpio/fb":
            print(result.hex())
        else:
            sys.stdout.buffer.write(result)
    return 0


if __name__ == "__main__":
    sys.exit(main())
