"""One-client UDP echo server for the PicoNIX networking tutorial (host PC)."""
import socket
import sys

port = int(sys.argv[1]) if len(sys.argv) > 1 else 12345
if not 1 <= port <= 65535:
    raise SystemExit("port must be 1..65535")
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
    sock.bind(("0.0.0.0", port))
    print(f"listening UDP {port}", flush=True)
    while True:
        payload, address = sock.recvfrom(512)
        print(f"{address}: {payload!r}", flush=True)
        sock.sendto(payload, address)
