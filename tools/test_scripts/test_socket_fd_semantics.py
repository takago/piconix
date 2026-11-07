#!/usr/bin/env python3
"""Regression checks for socket FD and UDP empty datagram semantics."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
kernel = (ROOT / "kernel/kernel.c").read_text()
backend = (ROOT / "kernel/net_socket.c").read_text()
assert re.search(r'if\s*\(\s*dev\s*==\s*DEV_SOCKET_TCP\s*&&\s*net_socket_tcp_listening\s*\(\s*id\s*\)\s*\)\s*return\s+OS_EBADF\s*;', kernel)
assert "n==-3 ? OS_EAGAIN" in kernel
assert "if (!s->pcb || !s->rx_ready) return -3;" in backend
assert re.search(r"return\s+s->peer_closed\s*\|\|\s*s->fault\s*\?\s*-2\s*:\s*0\s*;", backend)
print("PASS: TCP listener FD and UDP empty datagram semantics")
