"""Check MCP command capture can set O_NONBLOCK on an anonymous pipe."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
k = (root / 'kernel/kernel.c').read_text()
m = (root / 'userland/apps/mcpd.c').read_text()
start = k.index('case SYS_FCNTL:')
end = k.index('case SYS_LSEEK:', start)
f = k[start:end]
assert '!pipe_end(tasks[current].fd[a])' in f, 'F_SETFL must allow pipe FDs'
assert 'tasks[current].fd_flags[a]=(tasks[current].fd_flags[a] & ~OS_O_NONBLOCK)' in f
assert 'if(t->fd_flags[a] & OS_O_NONBLOCK) { result=OS_EAGAIN; break; }' in k
assert 'fcntl(fds[0],F_SETFL,O_NONBLOCK)' in m
assert 'read(fds[0],raw,sizeof raw)' in m
print('PASS: MCP capture pipe accepts F_SETFL(O_NONBLOCK) and read can return EAGAIN')
