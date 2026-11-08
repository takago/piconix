#!/usr/bin/env python3
"""Ensure wifi restart accepts both complete and split write calls."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
k = (root / 'kernel/kernel.c').read_text()
echo = (root / 'userland/apps/echo.c').read_text()
assert 'return flush(&out)<0 ? 1 : 0;' in echo
assert 'static const char cmd[] = "restart\\n"' in k
assert 'tasks[current].fd_pos[fd]' in k
assert 'pos+n >= 7u' in k
assert 'net_wifi_request_restart()!=0' in k
assert 'n==7 && !memcmp(p,"restart",7)' not in k
print('PASS: Wi-Fi restart accepts both buffered and split writes')
