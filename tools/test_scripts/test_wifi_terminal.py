#!/usr/bin/env python3
"""Static checks for the shared Telnet and raw TCP network TTY."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
net = (root / 'kernel/net.c').read_text()
kern = (root / 'kernel/kernel.c').read_text()
hdr = (root / 'kernel/net.h').read_text()
opts = (root / 'kernel/lwipopts.h').read_text()
for token in ('#define NET_TTY_QUEUE_SIZE 1024u',
              'if (needed > queue_space(net_tx_head, net_tx_tail)) return 0;',
              'if (available > TCP_MSS) available = TCP_MSS;',
              'if (tcp_close(pcb) != ERR_OK) tcp_abort(pcb);',
              'net_receive_byte(data[i]);', 'client_telnet', 'telnet_hello',
              'which ? 23 : 2323'):
    assert token in net, token
for token in ('wifi_session_poll();', 'if(wifi_session_active && epoch!=wifi_session_epoch)',
              'wifi_session_stop();', 'wifi_session_epoch=epoch;',
              'if(uartdev==2 && n!=0 && i==0)',
              'net_terminal_connected() ? OS_EAGAIN : OS_EIO'):
    assert token in kern, token
assert 'unsigned net_terminal_epoch(void);' in hdr
assert '#define TCP_MSS 536' in opts
assert re.search(r'#define\s+MEMP_NUM_TCP_PCB_LISTEN\s+2\b', opts)
assert net.count('++terminal_epoch;') >= 3
receive = net.split('static err_t client_recv(', 1)[1].split('\nstatic ', 1)[0]
assert 'tcp_write(' not in receive and 'tcp_output(' not in receive
print('PASS: Telnet/raw TCP, bounded queues and session lifecycle')
