#!/usr/bin/env python3
"""Regression checks for socket FD and UDP empty datagram semantics."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
kernel = (ROOT / "kernel/kernel.c").read_text()
backend = (ROOT / "kernel/net_socket.c").read_text()
assert "if(dev==DEV_SOCKET_TCP && net_socket_tcp_listening()) return OS_EBADF;" in kernel
assert "n==-3 ? OS_EAGAIN" in kernel
assert "if (!rx_ready) return -3;" in backend
assert "if (tcp_peer_closed && !tcp_rx_len) return -2;" in backend
print("PASS: TCP listener FD and UDP empty datagram semantics")
