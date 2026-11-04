#!/usr/bin/env python3
"""Check PicoNIX socket poll/error contracts and their bilingual documentation."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
kernel = (ROOT / 'kernel/kernel.c').read_text()
backend = (ROOT / 'kernel/net_socket.c').read_text()
lib = (ROOT / 'userland/lib/syscalls.c').read_text()

# No scheduler blocking path is used by socket read/write: the socket branch
# precedes all per-device operations and directly maps empty transfers to EAGAIN.
start = kernel.index('if(dev==DEV_SOCKET_TCP || dev==DEV_SOCKET_TCP_ACCEPTED)', kernel.index('static int vfs_rw('))
end = kernel.index('if(dev>=VDIR_ROOT', start)
tcp_rw = kernel[start:end]
assert 'r==0 ? OS_EAGAIN' in tcp_rw
assert 'writing ? OS_EIO : 0' in tcp_rw
assert 'net_socket_tcp_status(id)' in tcp_rw
assert 'BLOCKED' not in tcp_rw
assert 't->state=BLOCKED' not in tcp_rw
assert 'result=n==-2 ? OS_EINVAL : n==-3 ? OS_EAGAIN' in kernel
assert 'capacity < rx_length) return -2' in backend
assert 'if (!rx_ready) return -3' in backend
assert 'if (udp_socket || !port) return -1' in backend
assert 'if (!udp_socket) {' in backend
assert 'if(n>TCP_MSS) n=TCP_MSS;' in backend
assert 's->peer_closed || s->fault ? -2 : 0' in backend
assert 'if (flags || (len && !buf)) return OS_EINVAL;' in lib
assert 'if (addr || len) return OS_EINVAL;' in lib
assert 'if(fd_is_socket(t->fd[a])) { result=OS_EINVAL;' in kernel

for lang in ('ja', 'en'):
    guide = (ROOT / f'docs/{lang}/development/apps/c/NETWORK-PROGRAMMING.md').read_text()
    syscall = (ROOT / f'docs/{lang}/reference/SYSCALLS.md').read_text()
    network = (ROOT / f'docs/{lang}/reference/network/NETWORK.md').read_text()
    for content in (guide, syscall, network):
        for token in ('O_NONBLOCK', 'OS_EAGAIN', 'OS_EINVAL'):
            assert token in content, f'{lang}: missing {token}'
    for token in ('TCP_MSS', 'tcp_status()', 'shutdown(SHUT_WR)', 'dup()', 'OS_EBADF'):
        assert token in guide, f'{lang}: missing {token}'
    assert 'OS_EIO' in guide
print('PASS: socket I/O implementation and bilingual documentation contracts')
