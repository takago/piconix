#!/usr/bin/env python3
"""Source-level guards for Wi-Fi self-address loopback and terminal display."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
capture = (root / 'kernel/net_capture.c').read_text()
net = (root / 'kernel/net.c').read_text()
options = (root / 'kernel/lwipopts.h').read_text()
tcpdump = (root / 'userland/apps/tcpdump.c').read_text()
pong = (root / 'userland/apps/pong.c').read_text()
assert '#define LWIP_NETIF_LOOPBACK 1' in options
# lwIP itself queues self-addressed IPv4; only Wi-Fi queue polling is missing.
assert 'net_capture_attach(&cyw43_state.netif[' in net
assert 'return outputs[i].original(iface,p,ip)' in capture
assert 'netif_poll(loop_iface)' in net
assert 'netif_poll(&cyw43_state.netif[' in net
assert 'if(hex) printf(" DATA=%u",p->data_len);' in tcpdump
assert 'if(hex && p->sample_len)' in tcpdump
assert 'printf("    DATA:  ");' in tcpdump
assert 'printf("%02x ",p->sample[i]);' in tcpdump
assert tcpdump.index("putchar('\\n');") < tcpdump.index('printf("    DATA:  ");')
assert '#include "../lib/screen.h"' in pong
assert 'screen_measure(&terminal)' in pong
assert 'int top=terminal.rows>H?' in pong
assert 'int left_col=terminal.cols>W?' in pong
assert 'row(a,y,top,left_col,grid[y])' in pong
for language in ('en', 'ja'):
    assert '192.168.4.1' in (root / f'docs/{language}/reference/network/NETWORK.md').read_text()
    assert 'pong' in (root / f'docs/{language}/guides/TERMINAL.md').read_text()
print('PASS: self-IP loopback, two-line tcpdump -x sample, centered pong and documentation')
