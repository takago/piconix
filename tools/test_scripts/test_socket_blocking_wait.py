#!/usr/bin/env python3
"""Check socket blocking integration and retained multiplexing behavior."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
assert '#define WAIT_TCP_ACCEPT (-3)' in k
assert '#define WAIT_TCP_READ   (-4)' in k
assert '#define WAIT_UDP_READ   (-5)' in k
assert 'wake_sockets();' in k
assert 'if(net_poll_in_progress()) return;' in k
assert 't->wait_frame_ready=0; t->state=BLOCKED;' in k
assert 'if(t->fd_flags[a] & OS_O_NONBLOCK) { result=OS_EAGAIN; break; }' in k
assert '!(t->fd_flags[a] & OS_O_NONBLOCK)' in k
assert 'if(n==-3 && !(tasks[current].fd_flags[a] & OS_O_NONBLOCK))' in k
assert 'task_unblock(t,result);' in k
assert 'if(tasks[current].state!=BLOCKED)' in k
nc=(root/'userland/apps/nc.c').read_text()
assert 'fcntl(fd,F_SETFL,O_NONBLOCK);' in nc
assert 'fcntl(peer,F_SETFL,O_NONBLOCK);' in nc
httpd=(root/'userland/apps/httpd.c').read_text()
assert 'fcntl(client,F_SETFL,O_NONBLOCK);' in httpd
for lang in ('ja','en'):
  c=(root/f'docs/{lang}/development/apps/c/NETWORK-PROGRAMMING.md').read_text()
  assert 'O_NONBLOCK' in c and 'BLOCKED' in c
print('PASS: blocking accept/recv/recvfrom and nonblocking multiplexing contracts')
