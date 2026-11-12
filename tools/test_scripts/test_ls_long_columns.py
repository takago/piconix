#!/usr/bin/env python3
"""Keep ls -l columns stable across device ID and file size lengths."""
from pathlib import Path
s=(Path(__file__).resolve().parents[2]/'userland/apps/ls.c').read_text()
assert 'right_field(id_text,5)' in s
assert 'right_field(size_text,6)' in s
assert 'TYPE    ID   SIZE NAME' in s
assert 'snprintf(id_text,sizeof id_text,"%u",id)' in s
assert 'snprintf(size_text,sizeof size_text,"%u",size)' in s

def record(kind, number, size, name):
    return f'{kind:<4} {number:>5} {size:>6} {name}'
lines=[record('c', x, '-', n) for x,n in [(0,'ttyV0'),(278,'ttyV1'),(3,'dev'),(65535,'unknown')]]
lines += [record('f', '1', '0', 'a'),record('f','999','123456','b')]
assert len(set(line.index(line.split()[-1]) for line in lines))==1
assert lines[0].endswith('ttyV0')
print('PASS: ls -l fixed-width ID/SIZE fields')
