#!/usr/bin/env python3
"""Ensure terminal screen version is committed only after stream completion."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / 'userland/apps/mcpd.c').read_text()
block = source.split('static int terminal_screen_stream(', 1)[1].split('/* ROM内の/man', 1)[0]
assert block.count('term_last_hash=sent_hash;') == 1
assert block.count('term_screen_seen=1;') == 1
assert block.index('if(n<0) goto done;') > block.index('while((n=read(screen,row,sizeof row))>0)')
assert block.index('term_last_hash=sent_hash;') > block.index('if(send_all(fd,"0\\r\\n\\r\\n",5)<0) goto done;')
assert block.index('term_screen_seen=1;') < block.index('done:\n    close(screen);')
print('PASS: terminal screen state committed only after complete stream')
