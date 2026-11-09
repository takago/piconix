#!/usr/bin/env python3
"""Verify screen-embedded cursor; no separate cursor device or MCP coordinate prefix."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
m=(root/'userland/apps/mcpd.c').read_text()
d=(root/'include/device_names.h').read_text()
assert 'DEV_PTY_CURSOR' not in d and '0.cursor' not in d
assert 'DEV_PTY_CURSOR' not in k and 'EMIT("0.cursor"' not in k
assert '0.cursor' not in m and 'Cursor: row=' not in m
assert 'hash=(hash^(uint8_t)scan[i])*16777619u' in m
assert 'term_screen_seen && hash==term_last_hash' in m
assert 'send_escaped_chunk(fd,row,(unsigned)n)' in m
assert 'pty_screen_read(fd,p,n)' in k
print('PASS: cursor encoded in screen stream; redundant cursor node removed')
