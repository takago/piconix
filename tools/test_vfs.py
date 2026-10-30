#!/usr/bin/env python3
"""Verify minimal VFS uses the same fixed IDs for path lookup and FD names."""
from pathlib import Path
import re
k=(Path(__file__).resolve().parents[1]/'kernel/kernel.c').read_text()
section=k.split('static const struct { const char *path; int id; } vfs_registry[] = {',1)[1].split('};',1)[0]
entries=re.findall(r'\{"([^"\n]+)",(DEV_\w+)\}',section)
assert len(entries)==16 and len(set(p for p,_ in entries))==len(entries)
assert len(set(i for _,i in entries))==len(entries)
for path,dev in entries:
    assert path.startswith(('/bin','/tmp','/proc/','/dev/'))
assert 'fixed_path_id(p)' in k
assert 'if(vfs_registry[i].id==dev) return vfs_registry[i].path;' in k
for name in ('vfs_open(', 'vfs_rw(', 'tmp_open(', 'tmp_rw(', 'tmp_unlink('):
    assert name in k
print('PASS: VFS fixed-path registry and dispatch consistency')
