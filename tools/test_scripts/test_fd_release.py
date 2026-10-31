#!/usr/bin/env python3
"""Source-level guard for centralized descriptor teardown."""
from pathlib import Path
k=(Path(__file__).resolve().parents[2]/'kernel/kernel.c').read_text()
start=k.index('static void fd_release(')
end=k.index('/* Copy an open descriptor',start)
release=k[start:end]
assert 'pipe_ref(dev,-1);' in release
assert 'fd_set(t,fd,-1,0,0,0);' in release
for kind in ('DEV_SOCKET_UDP','DEV_SOCKET_TCP','DEV_SOCKET_TCP_ACCEPTED'):
    assert kind in release
assert 'for(int fd=0;fd<OS_FD_MAX;fd++) fd_release(t,fd);' in k
assert 'fd_release(&tasks[current],b);' in k
assert 'else { fd_release(&tasks[current],a); result=0; }' in k
assert 'pipe_fd_release' not in k
print('PASS: unified FD release source guards')
