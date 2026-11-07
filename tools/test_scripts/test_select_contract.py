#!/usr/bin/env python3
"""Verify new select does not consume TCP accept/UDP receive events and keeps FD ABI."""
from pathlib import Path
import re, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
n=(root/'kernel/net_socket.c').read_text()
u=(root/'userland/lib/syscalls.c').read_text()
s=(root/'include/piconix_syscall.h').read_text()
assert 'X(SYS_SELECT,        40, "select")' in s
assert 'net_socket_tcp_accept_peek(id)' in k and 'net_socket_tcp_recv_ready(id)' in k
assert 'net_socket_udp_read_ready(id)' in k
assert 't->wait_dev=WAIT_SELECT' in k and 'wake_selects();' in k
assert 'task_unblock(t,n)' in k
assert 'if(net_poll_in_progress()) return;' in k
assert 'int select(int nfds, fd_set *readfds' in u
assert 'net_socket_tcp_accept_ready(id)' not in k[k.index('static int select_ready('):k.index('static void wake_selects(')]
assert 'rx_ready = 0;' not in n[n.index('NO_TRACE int net_socket_udp_read_ready(int id)'):n.index('/* UDP待受ポート')]
# Validate mask operations and wrapper syntax on ARM ABI.
with tempfile.TemporaryDirectory() as tmp:
    cmd=['clang','--target=arm-none-eabi','-mcpu=cortex-m0plus','-mthumb','-Oz','-fsyntax-only',
         '-I'+str(root/'include'),'-I'+str(root/'userland/lib'),str(root/'userland/lib/syscalls.c'),
         str(root/'userland/apps/nc.c'),str(root/'userland/apps/httpd.c')]
    subprocess.run(cmd,check=True)
print('PASS: select ABI, non-consuming socket readiness, task wait/wake, ARM syntax')
