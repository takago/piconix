#!/usr/bin/env python3
"""Verify terminal_read streams the screen once and does not cache old states."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / 'userland/apps/mcpd.c').read_text()
block = source.split('static int terminal_screen_stream(', 1)[1].split('/* ROM内の/man', 1)[0]
assert block.count('open("/dev/pts/0.screen",O_RDONLY)') == 1
assert block.count('read(screen,row,sizeof row)') == 1
assert 'send_escaped_chunk(fd,row,(unsigned)n)' in block
assert 'if(n<0) goto done;' in block
assert 'if(send_all(fd,"0\\r\\n\\r\\n",5)<0) goto done;' in block
assert 'hash' not in block and 'unchanged' not in block
assert 'close(screen);' in block
print('PASS: terminal_read streams one screen read with no hash or stale cache')
