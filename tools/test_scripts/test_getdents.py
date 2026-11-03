#!/usr/bin/env python3
"""Check one shared directory ABI replaces userspace namespace knowledge."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
k=(r/'kernel/kernel.c').read_text()
u=(r/'userland/lib/unistd.h').read_text()
s=(r/'userland/lib/syscalls.c').read_text()
l=(r/'userland/apps/ls.c').read_text()
w=(r/'userland/apps/walk.h').read_text()
assert '#include "../include/piconix_syscall.h"' in k
assert 'case SYS_GETDENTS:' in k and 'vfs_getdents(' in k
assert 'case VDIR_ROOT:' in k and 'case VDIR_SYM:' in k
assert 'DEV_BIN_LIST' not in k and 'DEV_TMP_LIST' not in k
assert 'bin_list_read' not in k and 'tmp_list_read' not in k
assert 'if(dir>=0) {' in k
assert 'case VDIR_TASK:' in k and 'case VDIR_TMP:' in k
assert 'int getdents(int fd' in u and 'return call(SYS_GETDENTS,' in s
assert 'getdents(fd,&e,sizeof e)' in l
assert 'getdents(fd,&entry,sizeof entry)' in w
assert 'walk_stream(' not in w and 'pin_gpio(' not in w
assert '#include "../include/piconix_dirent.h"' in k
root = r

for app in ('trace', 'mem'):
    source = (root / 'userland/apps' / f'{app}.c').read_text()
    assert 'getdents(fd,' in source, app
    assert 'open("/bin"' in source, app
assert 'SYS_GETDENTS' in (root / 'userland/apps/trace.c').read_text()
assert 'int program_id(const char *name)' in u
assert 'return call(SYS_PROGRAM_ID,' in s
for app in ('shell', 'watch', 'trace'):
    source=(root/'userland/apps'/f'{app}.c').read_text()
    assert 'program_id(' in source, app
print('PASS: getdents API and centralized program-ID lookup')
