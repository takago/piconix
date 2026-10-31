#!/usr/bin/env python3
"""Continuous PicoNIX UDP demo: animate the 8x8 framebuffer and sample ADC.

No retries or request IDs. Stop with Ctrl+C; framebuffer is disabled on exit.
"""
import argparse
import socket
import time

from piconixctl import request

FB = "/dev/gpio/fb"
ADC = "/dev/adc/0"


def run(host: str, port: int, interval: float, timeout: float) -> None:
    def send(command: str, path: str, payload: bytes = b"") -> bytes:
        msg = (command + " " + path).encode("ascii")
        if command == "WRITE":
            msg += b"\n" + payload
        return request(host, port, msg, timeout)

    # Moving single LED, one row per byte, with a bit shifting across columns.
    send("ENABLE", FB)
    try:
        step = 0
        while True:
            frame = bytearray(8)
            frame[(step // 8) % 8] = 1 << (step % 8)
            send("WRITE", FB, bytes(frame))
            adc = send("READ", ADC).decode("ascii", errors="replace").strip()
            print(f"step={step:4d} ADC0={adc}", flush=True)
            step += 1
            time.sleep(interval)
    finally:
        try:
            send("DISABLE", FB)
        except (OSError, RuntimeError) as exc:
            print(f"Warning: could not disable framebuffer: {exc}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", default="192.168.4.1")
    parser.add_argument("--port", type=int, default=2000)
    parser.add_argument("--interval", type=float, default=0.2)
    parser.add_argument("--timeout", type=float, default=1.0)
    args = parser.parse_args()
    if args.interval <= 0 or args.timeout <= 0:
        parser.error("interval and timeout must be positive")
    try:
        run(args.host, args.port, args.interval, args.timeout)
    except KeyboardInterrupt:
        print("Stopped")
    except (OSError, RuntimeError) as exc:
        parser.exit(1, f"UDP error: {exc}\n")


if __name__ == "__main__":
    main()
