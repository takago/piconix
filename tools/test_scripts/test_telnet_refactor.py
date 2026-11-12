#!/usr/bin/env python3
"""Check shared Telnet ring advancement and absence of unused parser state."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
src = (root / "kernel/net_service_telnet.c").read_text()
assert 'static unsigned queue_next(unsigned index)' in src
assert 'return (index + 1u) & (NET_TTY_QUEUE_SIZE - 1u);' in src
assert 'net_rx_tail = queue_next(net_rx_tail);' in src
assert src.count('net_tx_head = queue_next(net_tx_head);') == 2
assert 'net_rx_head = queue_next(net_rx_head);' in src
assert 'telnet_command' not in src
assert '#define NET_TTY_QUEUE_SIZE 2048u' in src
print('PASS: Telnet ring advancement is shared and parser state is minimal')
