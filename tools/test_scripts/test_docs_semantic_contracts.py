#!/usr/bin/env python3
"""Doc regressions for resource limits, interactive commands and current C syntax."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
kernel = (root / 'kernel/kernel.c').read_text()
net = (root / 'kernel/net_socket.h').read_text()
assert re.search(r'#define NET_SOCKET_UDP_SLOTS 3\b', net)
assert '!pipe_end(tasks[current].fd[a])' in kernel
for lang in ('ja', 'en'):
    base = root / 'docs' / lang
    cc = (base / 'development/apps/c/CC.md').read_text()
    assert cc.startswith('# ') and '\n# ' not in cc
    assert all(needle in cc for needle in ('256', '2048', '1024', '++', '--', 'int *', 'char *'))
    assert '64-byte stack frame' not in cc and '64バイトの局所領域' not in cc
    pipes = (base / 'reference/commands/PIPE.md').read_text()
    tty = (base / 'reference/commands/TTY.md').read_text()
    assert 'F_SETFL' in pipes and 'OS_EAGAIN' in pipes
    assert 'O_NONBLOCK' in tty and ('pipe' in tty.lower() or 'パイプ' in tty)
    cnet = (base / 'development/apps/c/NETWORK-PROGRAMMING.md').read_text()
    pynet = (base / 'development/apps/python/SOCKET-PROGRAMMING.md').read_text()
    assert '3' in cnet and '3' in pynet
    cmd = (base / 'reference/commands/README.md').read_text()
    assert '[`reversi`]' in cmd and '(REVERSI.md)' in cmd
    mcp = (base / 'reference/commands/MCP.md').read_text()
    assert 'reversi' in mcp and 'execute_command' in mcp
    rev = (base / 'reference/commands/REVERSI.md').read_text()
    assert all(item in rev for item in ('terminal_read', 'terminal_send', 'Invalid moves'))
    idx = (base / 'development/FUNCTIONS.md').read_text()
    assert 'userland/apps/reversi.c' in idx
    assert idx.count('`__attribute__()`') == 0
for topic in ('README', 'agent', 'command-reference', 'safety-recovery', 'pty', 'mcp-workflow', 'reversi'):
    assert 'reversi' in (root / 'manual' / topic).read_text().lower(), topic
assert '++/-- is supported' in (root / 'manual/languages/c').read_text()
print('PASS: semantic documentation contracts (C, pipes, UDP, MCP and indexes)')
