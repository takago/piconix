#!/usr/bin/env python3
"""Source-level regression for PTY key aliases and send-screen response."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
k = (root / "kernel/kernel.c").read_text()
m = (root / "userland/apps/mcpd.c").read_text()
assert '!strcmp(key,"ESCAPE")' in k
assert '!strcmp(key,"BS")' in k
assert '!strcmp(key,"SPACE") || !strcmp(key,"SP")' in k
assert 'ESCAPE TAB SPACE SP BACKSPACE BS DELETE' in m
send = m.split('if(term_master<0) {rpc_error(id,-32030,"No PTY session")',1)[1]
send = send.split('if (json_equals(v,v_end,"get_manual"))',1)[0]
assert 'for(int i=0;i<20;i++)' in send
assert 'terminal_drain(32,0);' in send
assert 'msleep(10);' in send
assert send.index('terminal_drain(32,0);') < send.index('return terminal_screen_stream(fd,id);')
assert 'if(wait_empty && i<4) msleep(10);else break;' in m
assert 'term_screen_seen=0;' in send
assert 'return terminal_screen_stream(fd,id);' in send
assert 'Unknown key.' in send
assert 'After sending, waits 200ms then returns ANSI screen (SGR 4/24 marks cursor; 7/0 inverse)' in m
assert 'Read ANSI screen: SGR 4/24 marks cursor; SGR 7/0 marks inverse' in m
print('PASS: PTY key aliases and terminal_send screen return')
