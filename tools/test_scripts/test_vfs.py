#!/usr/bin/env python3
"""VFS static lookup and dispatch coverage after splitting the path registry."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
v=k
h=k
section=v.split('static const struct { const char *path; int id; } vfs_registry[] = {',1)[1].split('};',1)[0]
entries=re.findall(r'\{"([^"\n]+)",(DEV_\w+)\}',section)
assert len(entries)==14 and len(set(p for p,_ in entries))==len(entries)
assert len(set(i for _,i in entries))==len(entries)
for path,dev in entries: assert path.startswith(('/bin','/tmp','/proc/','/dev/'))
assert 'vfs_fixed_path_id(p)' in k and 'vfs_directory_id(p)' in k
assert 'vfs_fixed_id_path(dev)' in k and 'vfs_directory_path(dev)' in k
assert 'kernel/vfs.c' not in (root/'CMakeLists.txt').read_text()
assert 'enum { VDIR_ROOT=300' in h
for name in ('vfs_open(', 'vfs_rw(', 'tmp_open(', 'tmp_rw(', 'tmp_unlink(', 'vfs_getdents('): assert name in k
print('PASS: integrated VFS path registry and dispatch consistency')
