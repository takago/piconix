#!/usr/bin/env python3
"""STA GARP is periodically sent from the existing network poll, never ISR."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
s=(r/'kernel/net.c').read_text()
assert '#include "lwip/etharp.h"' in s
assert '#define STA_GARP_INTERVAL_US 60000000ull' in s
assert 'err_t err = etharp_gratuitous(iface);' in s
assert 'sta_garp_next_us = now + STA_GARP_INTERVAL_US;' in s
# DHCP/リンク喪失時にGARP予約を取り消す実装を検査する．
assert 'sta_rssi_next_us = 0;\n    sta_garp_next_us = 0;' in s
assert s.index('cyw43_arch_poll();',s.index('void net_poll(void)')) < s.index('sta_poll();',s.index('void net_poll(void)'))
assert 'sta_tcpip_link == CYW43_LINK_UP && sta_dhcp_bound' in s
for key in ('GARP_INTERVAL_S:', 'GARP_OK:', 'GARP_ERROR:'):
    assert key in s
print('PASS: STA-only periodic gratuitous ARP and diagnostics')
