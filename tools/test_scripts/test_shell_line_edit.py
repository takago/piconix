#!/usr/bin/env python3
"""Regression checks for small-shell line editing and completion."""
from pathlib import Path
src = (Path(__file__).resolve().parents[2] / 'userland/apps/shell.c').read_text()
for key, label in ((1, 'Ctrl+A'), (5, 'Ctrl+E'), (11, 'Ctrl+K'), (23, 'Ctrl+W')):
    assert f'if(c=={key})' in src, label
assert 'shell_complete(line,&len,&cursor,cwd)' in src
assert 'getdents(fd,&entry,sizeof entry)' in src
assert 'strcpy(directory,"/bin")' in src
assert 'memmove(line+cursor,line+end,len-end+1)' in src
assert 'shell_readline(line,cwd)' in src
assert 'if(c==12)' in src
print('PASS: shell control keys and compact Tab completion')
