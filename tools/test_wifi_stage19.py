#!/usr/bin/env python3
"""Static checks for TCP session identity and shell teardown."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
n=(r/'kernel/net.c').read_text()
k=(r/'kernel/kernel.c').read_text()
h=(r/'kernel/net.h').read_text()
assert 'unsigned net_terminal_epoch(void);' in h
assert n.count('++terminal_epoch;') == 3
assert 'if(wifi_session_active && epoch!=wifi_session_epoch)' in k
assert 'wifi_session_stop();' in k
assert 'wifi_session_epoch=epoch;' in k
print('PASS: session epoch on accept, FIN, error; stale shell teardown')
