#!/usr/bin/env python3
"""Catch accidental duplicate non-Wi-Fi stubs and mismatched TCP syscall IDs."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
src = (root/'kernel/net_socket.c').read_text()
nonwifi = src.split('\n#else\n',1)[1].split('\n#endif',1)[0]
for name in ('net_socket_tcp_accept_ready','net_socket_tcp_listening','net_socket_tcp_close_listener','net_socket_tcp_close_client'):
    assert len(re.findall(r'\b'+name+r'\s*\(', nonwifi)) == 1, name
u = (root/'userland/syscalls.c').read_text()
for name, number in [('tcp_listen',32),('tcp_connect',33),('tcp_status',34),('tcp_accept',35)]:
    assert re.search(r'\bint\s+'+name+r'\s*\([^}]+call\('+str(number)+r',',u),name
print('PASS: socket non-Wi-Fi stubs and userland syscall wrappers')
