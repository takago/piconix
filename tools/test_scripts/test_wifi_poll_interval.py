#!/usr/bin/env python3
"""Guard the experimental 250 us CYW43 polling throttle."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
net = (root / 'kernel/net.c').read_text()
main = (root / 'kernel/main.c').read_text()
assert '#define NET_POLL_INTERVAL_US 250ull' in net
poll = net.split('void net_poll(void) {', 1)[1].split('\n}', 1)[0]
assert 'now - net_poll_last_us < NET_POLL_INTERVAL_US) return;' in poll
assert poll.index('now - net_poll_last_us < NET_POLL_INTERVAL_US') < poll.index('++net_poll_count;') < poll.index('cyw43_arch_poll();')
assert poll.index('cyw43_arch_poll();') < poll.index('sta_poll();')
assert 'net_poll();' in main
print('PASS: 250us network polling throttle and counter order')
