#!/usr/bin/env python3
"""Source-level guards for the small, blocking PicoNIX waitpid design."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
k = (root / 'kernel/kernel.c').read_text()
s = (root / 'userland/apps/shell.c').read_text()
u = (root / 'userland/lib/unistd.h').read_text()
assert 't->wait_dev=-2;' in k
assert 'wake_waiters();' in k
assert 'task_unblock(t,pid);' in k
assert 't->sp[8]=(uint32_t)result;' in k
assert 'take_exit(tasks[current].pid,a)' in k
assert 'if(c & 1) { result=0; break; }' in k
assert 'if(!running) { result=OS_ESRCH; break; }' in k
assert '#define WNOHANG 1' in u
assert 'while(waitpid(pid' not in s
assert 'while(waitpid(-1, &bg_status, 1)>0)' in s
assert '---- sh built-in commands ----' in (root / 'userland/apps/ls.c').read_text()
assert '[help]\\r\\n' not in s
print('PASS: blocking waitpid, nonblocking child reap, and /bin built-in display')
