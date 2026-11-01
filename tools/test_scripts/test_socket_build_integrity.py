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
assert '#include "../include/piconix_syscall.h"' in u
for name, ident in [('tcp_listen','SYS_LISTEN_TCP'),('tcp_connect','SYS_CONNECT_TCP'),('tcp_status','SYS_TCP_STATUS'),('tcp_accept','SYS_ACCEPT_TCP')]:
    assert re.search(r'\bint\s+'+name+r'\s*\([^}]+call\('+ident+r',',u),name
print('PASS: socket non-Wi-Fi stubs and userland syscall wrappers')
