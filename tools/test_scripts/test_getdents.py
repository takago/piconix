#!/usr/bin/env python3
"""Check one shared directory ABI replaces userspace namespace knowledge."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
k=(r/'kernel/kernel.c').read_text()
u=(r/'userland/unistd.h').read_text()
s=(r/'userland/syscalls.c').read_text()
l=(r/'userland/ls.c').read_text()
w=(r/'userland/walk.h').read_text()
assert 'SYS_SPAWN_TMP, SYS_GETDENTS' in k
assert 'case SYS_GETDENTS:' in k and 'vfs_getdents(' in k
assert 'case VDIR_ROOT:' in k and 'case VDIR_SYM:' in k
assert 'DEV_BIN_LIST' not in k and 'DEV_TMP_LIST' not in k
assert 'bin_list_read' not in k and 'tmp_list_read' not in k
assert 'if(dir>=0) {' in k
assert 'case VDIR_TASK:' in k and 'case VDIR_TMP:' in k
assert 'int getdents(int fd' in u and 'return call(26,' in s
assert 'getdents(fd,&e,sizeof e)' in l
assert 'getdents(fd,&entry,sizeof entry)' in w
assert 'walk_stream(' not in w and 'pin_gpio(' not in w
assert '#include "../include/piconix_dirent.h"' in k
root = r

for app in ('shell', 'watch', 'trace', 'mem'):
    source = (root / 'userland' / f'{app}.c').read_text()
    assert 'getdents(fd,' in source, app
    assert 'open("/bin"' in source, app
assert 'case 26: return "getdents";' in (root / 'userland/trace.c').read_text()
print('PASS: getdents API, centralized namespace and consumers')
