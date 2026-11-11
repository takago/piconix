#!/usr/bin/env python3
"""MCP screen-only send/read separation and PTY key aliases."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
m=(root/'userland/apps/mcpd.c').read_text()
for alias in ('ESCAPE','BS','SPACE','SP'):
    assert f'"{alias}"' in k
assert 'ESCAPE TAB SPACE SP BACKSPACE BS DELETE' in m
assert 'ioctl(master,OS_PTY_SCREEN_ONLY,1)' in m
assert 'terminal_drain(' not in m
assert 'After sending, waits 200ms' not in m
send=m.split('int state=terminal_ensure_shell();',1)[1].split('if (json_equals(v,v_end,"get_manual"))',1)[0]
assert 'accepted: %d bytes' in send
assert 'return terminal_screen_stream(fd,id)' not in send
assert 'rpc_result(id,reply);' in send
assert 'if(!strcmp(expected,"terminal_read")) return terminal_screen_stream(fd,id)' in m
assert 'if(terminal_start_shell()<0)' in m
assert 'if(term_master>=0)terminal_close_session();' in m
print('PASS: MCP input and screen retrieval separated; screen-only mode enabled')
