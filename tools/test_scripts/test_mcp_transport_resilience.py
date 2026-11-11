#!/usr/bin/env python3
"""Exercise real mcpd framing and retry code with partial writes and EAGAIN."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
src = (root / "userland/apps/mcpd.c").read_text()
net = (root / "kernel/net_socket.c").read_text()
assert 'TCP_CLOSE_DEFERRED:' in net
assert 'tcp_diag.write_backpressure++' in net
assert 'if(send_all(fd,"0\\r\\n\\r\\n",5)<0)' in src
assert 'send_fail_reason=send_fail_value=0;' in src
start = src.index('static int send_all(')
end = src.index('/* HTTPレスポンスを組み立てて返す． */', start)
chunk_start = src.index('static int send_chunk(')
chunk_end = src.index('/* バイト列をJSON文字列の内容として送り出す．', chunk_start)
code = src[start:end] + src[chunk_start:chunk_end]
harness = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OS_EAGAIN (-11)
static int send_fail_reason,send_fail_value;
static char response[900];
static int mode, calls, waits;
static char sent[2048];
static unsigned used;
int send(int fd,const char *p,unsigned n,int flags) {
    (void)fd; (void)flags;
    ++calls;
    if(mode==1) return 0;
    if(mode==2) return -99;
    if(calls%4==0) return OS_EAGAIN;
    if(n>7) n=7;
    assert(used+n<sizeof sent);
    memcpy(sent+used,p,n); used+=n;
    return (int)n;
}
static int wait_fd(int fd,int writing,int timeout_ms) { assert(fd==1 && writing==1 && timeout_ms==2); ++waits; return 0; }
''' + code + r'''
int main(void) {
    const char *payload="{\"hello\":\"test\"}";
    assert(send_chunk(1,payload,(unsigned)strlen(payload))==0);
    assert(send_all(1,"0\r\n\r\n",5)==0);
    assert(used==strlen(payload)+11);
    assert(!memcmp(sent,"10\r\n",4));
    assert(!memcmp(sent+4,payload,strlen(payload)));
    assert(!memcmp(sent+used-7,"\r\n0\r\n\r\n",7));
    assert(waits>0);
    mode=1;send_fail_reason=0;
    assert(send_all(1,"x",1)==-1);
    assert(send_fail_reason==1 && send_fail_value==1);
    mode=2;send_fail_reason=0;
    assert(send_all(1,"z",1)==-1);
    assert(send_fail_reason==2 && send_fail_value==-99);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    path = Path(tmp) / "transport.c"
    binary = Path(tmp) / "transport"
    path.write_text(harness)
    subprocess.run(["cc", "-std=c99", "-Wall", "-Wextra", "-Werror", str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("PASS: MCP chunk framing, partial writes, EAGAIN, timeout/error classification")
