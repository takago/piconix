#!/usr/bin/env python3
"""Static regression checks for Wi-Fi TTY transport invariants."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
net = (root / 'kernel/net.c').read_text()
kern = (root / 'kernel/kernel.c').read_text()
assert '#define NET_TTY_QUEUE_SIZE 1024u' in net
assert '#define TCP_MSS 536' in (root / 'kernel' / 'lwipopts.h').read_text()
assert 'if (size > queue_space(net_tx_head, net_tx_tail)) return 0;' in net
assert 'if (tcp_close(pcb) != ERR_OK) tcp_abort(pcb);' in net
assert 'uint8_t b[1]; int n;' in kern
assert 'wifi_session_poll();' in kern
print('PASS: full-MSS receive, atomic CRLF writes, TCP close fallback, bounded TTY input')
