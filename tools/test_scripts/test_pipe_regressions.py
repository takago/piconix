#!/usr/bin/env python3
"""Source-level regression guards; not a firmware or runtime test."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
s=(root/'userland/shell.c').read_text()
assert 'if(a!=b) {' in k and 'pipe_fd_release(&tasks[current],b);' in k
assert 'fd_copy(&tasks[current],b,a);' in k
assert 'fd_copy(t,target,a);' in k
assert 'pipe_ref(t->fd[src],1);' in k
assert 'wake_pipes();' in k and 't->sp[8]=(uint32_t)result;' in k
assert 'for(int fd=0;fd<OS_FD_MAX;fd++) pipe_fd_release(t,fd);' in k
assert 'saved_in=dup(0);' in s and 'saved_out=dup(1);' in s
print('PASS: pipe source-level regression guards (not execution tests)')
