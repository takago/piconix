#!/usr/bin/env python3
"""Exercise the actual mcpd select helper with deterministic readiness stubs."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'userland/apps/mcpd.c').read_text()
assert 'msleep(' not in s
assert 'wait_fd(server,0,-1)' in s
assert 'wait_fd(fd,0,5)' in s
assert 'wait_fd(fd,1,2)' in s
assert 'wait_fd(eof?-1:fds[0],0,10)' in s
start=s.index('static int wait_fd(')
end=s.index('/* 非ブロッキング送信',start)
helper=s[start:end]
harness=r'''#include <assert.h>
#include <stdint.h>
typedef uint16_t fd_set;
#define FD_SET(fd,p) (*(p) |= (uint16_t)(1u << (fd)))
struct timeval { long tv_sec, tv_usec; };
static int expected_fd, expected_write, expected_ms;
static int select(int nfds,fd_set *readfds,fd_set *writefds,fd_set *exceptfds,const struct timeval *timeout) {
    (void)exceptfds;
    assert(nfds==expected_fd+1);
    if(expected_fd>=0) {
        assert((readfds==0)==expected_write);
        assert((writefds!=0)==expected_write);
    }
    if(expected_fd>=0) assert(*(expected_write?writefds:readfds)==(1u<<expected_fd));
    else assert(!readfds && !writefds);
    if(expected_ms<0) assert(!timeout);
    else assert(timeout && timeout->tv_sec==expected_ms/1000 && timeout->tv_usec==(expected_ms%1000)*1000);
    return expected_ms==0 ? 0 : 1;
}
''' + helper + r'''
int main(void) {
    expected_fd=5;expected_write=0;expected_ms=-1;
    assert(wait_fd(5,0,-1)==1);
    expected_fd=6;expected_write=1;expected_ms=2;
    assert(wait_fd(6,1,2)==1);
    expected_fd=3;expected_write=0;expected_ms=3005;
    assert(wait_fd(3,0,3005)==1);
    expected_fd=-1;expected_write=0;expected_ms=10;
    assert(wait_fd(-1,0,10)==1);
    assert(wait_fd(16,0,5)<0);
    assert(wait_fd(-1,0,-1)<0);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    c=Path(d)/'select.c'; exe=Path(d)/'select'
    c.write_text(harness)
    subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: mcpd select helper for accept, socket I/O and pipe wait')
