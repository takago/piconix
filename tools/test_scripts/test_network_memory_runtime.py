#!/usr/bin/env python3
"""Compile the real /proc/net/memory formatter with mocked lwIP stats."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
net = (root / 'kernel/net.c').read_text()
# 実装本体を抽出し，コメントの文言に依存しない．
start = net.index('int net_memory_info(')
brace = net.index('{', start)
depth = 0
for end in range(brace, len(net)):
    if net[end] == '{':
        depth += 1
    elif net[end] == '}':
        depth -= 1
        if depth == 0:
            break
else:
    raise AssertionError('net_memory_info(): missing closing brace')
formatter = net[start:end + 1]
mock = r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define MEMP_PBUF_POOL 1
#define MEMP_TCP_PCB 2
#define MEMP_TCP_SEG 3
#define MEM_SIZE (10 * 1024)
#define PBUF_POOL_SIZE 8
struct stats_mem { unsigned avail, used, max, err; };
struct { struct stats_mem mem; struct stats_mem *memp[4]; } lwip_stats;
int net_socket_tcp_diagnostics(char *out, unsigned size) {
    return snprintf(out,size,"TCP_ACCEPT_BUSY: 5\nTCP_CLOSE_DEFERRED: 3\n");
}
unsigned os_idle_stack_used_bytes(void) { return 1920; }
unsigned os_idle_stack_capacity_bytes(void) { return 4096; }
'''
main = r'''
int main(void) {
    char text[448] = {0};
#if PICONIX_WIFI
    struct stats_mem pool = {8, 3, 6, 2};
    struct stats_mem pcbs = {5, 2, 4, 1};
    struct stats_mem segs = {8, 1, 3, 0};
    lwip_stats.memp[MEMP_PBUF_POOL] = &pool;
    lwip_stats.memp[MEMP_TCP_PCB] = &pcbs;
    lwip_stats.memp[MEMP_TCP_SEG] = &segs;
    lwip_stats.mem.used = 1024;
    lwip_stats.mem.max = 3072;
    lwip_stats.mem.err = 1;
    assert(net_memory_info(text, sizeof(text)) > 0);
    assert(strstr(text, "LWIP_HEAP: 1024/3072/10240\n"));
    assert(strstr(text, "LWIP_HEAP_ERRORS: 1\n"));
    assert(strstr(text, "PBUF_POOL: 3/6/8\n"));
    assert(strstr(text, "PBUF_POOL_ERRORS: 2\n"));
    assert(strstr(text, "TCP_PCB_POOL: 2/4/1\n"));
    assert(strstr(text, "TCP_SEG_POOL: 1/3/0\n"));
    assert(strstr(text, "TCP_ACCEPT_BUSY: 5\n"));
    assert(strstr(text, "TCP_CLOSE_DEFERRED: 3\n"));
#else
    assert(net_memory_info(text, sizeof(text)) > 0);
    assert(strstr(text, "MODE: N/A\n"));
#endif
    assert(strstr(text, "IDLE_STACK: 1920/4096\n"));
    assert(net_memory_info(text, 0) == 0);
    assert(net_memory_info(text, 5) == 4);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='piconix_netmemory_') as d:
    p = Path(d)
    source = p / 'mem.c'
    source.write_text(mock + '\n' + formatter + '\n' + main)
    for wifi in (0, 1):
        subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
                        f'-DPICONIX_WIFI={wifi}', str(source),'-o',str(p/'test')],check=True)
        subprocess.run([str(p/'test')], check=True)
print('PASS: real network-memory formatter, AP/STA data and non-Wi-Fi fallback')
