#!/usr/bin/env python3
from pathlib import Path
r=Path(__file__).resolve().parents[2]
k=(r/'kernel/kernel.c').read_text()
s=(r/'userland/apps/shell.c').read_text()
u=(r/'userland/lib/unistd.h').read_text()
assert '#define OS_FD_MAX 16' in k
for name in ('fd','fd_pos','fd_task_pid','fd_flags'):
    assert f'{name}[OS_FD_MAX]' in k
assert 'case SYS_DUP:' in k and 'int dup(int oldfd)' in u
assert 'saved_in=dup(0);' in s and 'saved_out=dup(1);' in s
assert 'saved=dup(target);' in s
assert 'for(int fd=0;fd<OS_FD_MAX;fd++) fd_release(t,fd);' in k
print('PASS: FD16 and dynamic dup contract (static only)')
