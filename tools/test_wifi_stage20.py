#!/usr/bin/env python3
"""Static regression checks for TCP write backpressure handling."""
from pathlib import Path
r = Path(__file__).resolve().parents[1]
k = (r/'kernel/kernel.c').read_text()
n = (r/'kernel/net.c').read_text()
assert 'if(uartdev==2 && n!=0 && i==0)' in k
assert 'net_terminal_connected() ? OS_EAGAIN : OS_EIO' in k
assert 'if (available > TCP_MSS) available = TCP_MSS;' in n
assert 'if (size > queue_space(net_tx_head, net_tx_tail)) return 0;' in n
assert 'if(wrote!=1+insert_cr) break;' in k
print('PASS: zero-byte network writes report backpressure; MSS-bounded flush')
