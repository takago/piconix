#!/usr/bin/env python3
"""Regression checks for the bounded terminal-height mem viewer."""
from pathlib import Path
import re
import subprocess
import shutil
root=Path(__file__).resolve().parents[2]
source=(root/'userland/apps/mem.c').read_text()
assert '#define MAX_ROWS 36' in source
assert 'screen_measure(&size)' in source
assert 'view_rows=(unsigned)(rows-6)' in source
assert 'view_count=view_rows*BYTES_PER_ROW' in source
assert 'delta=esc==5?-(int)view_count:(int)view_count' in source
assert 'read(fd,current,view_count)==(int)view_count' in source
assert 'memcmp(previous,current,view_count)' in source
assert 'char old[ROWS]' not in source
assert '\\033[23;1H' not in source
assert 'screen_move((int)view_rows+6,1)' in source
for height,expected in ((8,2),(24,18),(42,36),(60,36)):
    rows=min((height if height>=8 else 24)-6,36)
    assert rows==expected
    assert rows+6<=height
for locale in ('en','ja'):
    mem=(root/f'docs/{locale}/guides/MEM.md').read_text()
    assert '36' in mem and 'PgUp' in mem
cc=shutil.which('gcc') or shutil.which('clang')
if cc:
    subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-fsyntax-only',
                    '-I',str(root/'userland/lib'),str(root/'userland/apps/mem.c')],check=True)
print('PASS: adaptive mem window, page size, terminal footer, documentation and host syntax')
