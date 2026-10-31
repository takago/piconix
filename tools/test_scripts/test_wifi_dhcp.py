#!/usr/bin/env python3
"""Static checks for DHCP response format and deferred transmission."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
net = (root / 'kernel/net.c').read_text()
opts = (root / 'kernel/lwipopts.h').read_text()
m = re.search(r'dhcp_reply_options\[\]\s*=\s*\{([^}]+)\}', net)
assert m, 'DHCP options missing'
options = [int(x.strip()) for x in m.group(1).split(',') if x.strip()]
assert options[-1] == 255 and options[:3] == [53, 1, 0]
assert 240 + len(options) <= 300
for token in ('unsigned char reply[DHCP_REPLY_LEN]', 'if (message_type) goto done;', 'dhcp_pending_reply', 'dhcp_send_pending('):
    assert token in net, token
assert 'LWIP_IP_ACCEPT_UDP_PORT(port)' in opts and 'PP_NTOHS(67)' in opts
receive = net.split('static void dhcp_recv(', 1)[1].split('\nstatic ', 1)[0]
assert 'udp_sendto(' not in receive, 'send inside DHCP callback can freeze Pico W'
assert 'udp_sendto(' in net
print('PASS: DHCP options, broadcast reception and deferred transmission')
