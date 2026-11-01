#!/usr/bin/env python3
"""Static checks for the Telnet network TTY."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
net = (root / 'kernel/net_service_telnet.c').read_text()
entry = (root / 'kernel/net.c').read_text()
assert 'net_service_telnet_poll();' in entry
kern = (root / 'kernel/kernel.c').read_text()
hdr = (root / 'kernel/net.h').read_text()
opts = (root / 'kernel/lwipopts.h').read_text()
libc = (root / 'userland/libc.c').read_text()
for token in ('#define NET_TTY_QUEUE_SIZE 2048u',
              'if (needed > queue_space(net_tx_head, net_tx_tail)) return 0;',
              'if (available > TCP_MSS) available = TCP_MSS;',
              'if (tcp_close(pcb) != ERR_OK) tcp_abort(pcb);',
              'net_receive_byte(data[i]);', 'client_telnet', 'telnet_hello',
              'tcp_bind(telnet_listener, IP_ANY_TYPE, 23)'):
    assert token in net, token
for token in ('wifi_session_poll();', 'if(wifi_session_active && epoch!=wifi_session_epoch)',
              'wifi_session_stop();', 'wifi_session_epoch=epoch;',
              'if(uartdev==2 && n!=0 && i==0)',
              'net_terminal_connected() ? OS_EAGAIN : OS_EIO'):
    assert token in kern, token
# Separate TX/RX rings each have 2047 usable bytes with the one-empty-slot convention.
assert 'static unsigned char net_rx[NET_TTY_QUEUE_SIZE], net_tx[NET_TTY_QUEUE_SIZE];' in net
assert net.count('(NET_TTY_QUEUE_SIZE - 1u)') >= 5
assert 'return NET_TTY_QUEUE_SIZE - 1u - queue_count(head, tail);' in net
assert 'if (p->tot_len > queue_space(net_rx_head, net_rx_tail))' in net
assert 'unsigned net_terminal_epoch(void);' in hdr
assert 'if(r==OS_EAGAIN || r==0){msleep(1);continue;}' in libc
assert '#define TCP_MSS 536' in opts
listen_match = re.search(r'#define\s+MEMP_NUM_TCP_PCB_LISTEN\s+(\d+)\b', opts)
assert listen_match and int(listen_match.group(1)) >= 5, 'TCP listen PCB pool must cover Telnet and four user sockets'
assert net.count('++terminal_epoch;') >= 3
receive = net.split('static err_t client_recv(', 1)[1].split('\nstatic ', 1)[0]
assert 'tcp_write(' not in receive and 'tcp_output(' not in receive
print('PASS: Telnet, bounded queues and session lifecycle')
