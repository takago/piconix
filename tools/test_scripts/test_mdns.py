#!/usr/bin/env python3
"""Source guards for STA/AP mDNS registration."""
from pathlib import Path
r = Path(__file__).resolve().parents[2]
n = (r / 'kernel/net.c').read_text()
o = (r / 'kernel/lwipopts.h').read_text()
c = (r / 'CMakeLists.txt').read_text()
assert 'pico_lwip_mdns' in c
assert '#define LWIP_MDNS_RESPONDER 1' in o
assert '#define LWIP_IGMP 1' in o
assert '#define LWIP_NUM_NETIF_CLIENT_DATA 1' in o
assert 'mdns_resp_init();' in n
assert 'mdns_resp_add_netif(iface, sta_mdns_name)' in n
assert 'mdns_resp_add_netif(ap_iface, "piconix")' in n
assert 'netif_set_addr(ap_iface, &ip, &mask, &gw);' in n
assert 'MDNS_NAME: piconix.local' in n
assert 'mdns_resp_remove_netif(' in n
assert 'mdns_resp_restart(iface)' in n
assert 'piconix-' in n and 'piconix_flash_id()' in n
assert 'MDNS_ACTIVE:' in n and 'MDNS_NAME:' in n
print('PASS: STA per-device and AP fixed-name mDNS guards')
