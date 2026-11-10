#!/usr/bin/env python3
"""Verify opt-in output-ring bypass does not alter normal PTY semantics."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
m=(root/'userland/apps/mcpd.c').read_text()
h=(root/'include/piconix.h').read_text()
assert '#define OS_PTY_SCREEN_ONLY 0x20b' in h
assert 'if(dev!=DEV_PTY_MASTER) return OS_EBADF;' in k
assert 'if(arg!=1 || pty.slave_refs || pty.used) return OS_EBUSY;' in k
assert 'if(!pty.screen_only)' in k
assert 'pty_screen_byte(c);' in k
assert '(!pty.screen_only && PTY_OUT_BYTES-pty.used < (unsigned)(1+insert_cr))' in k
assert '(tty_echo[PTY_TTY] && !pty.screen_only && PTY_OUT_BYTES-pty.used<8u)' in k
assert m.index('ioctl(master,OS_PTY_SCREEN_ONLY,1)') < m.index('int slave=open("/dev/pts/0"')
assert 'terminal_drain(' not in m
assert 'return terminal_screen_stream(fd,id);' in m
assert 'accepted: %d bytes' in m
print('PASS: screen-only opt-in and generic PTY compatibility checks')
