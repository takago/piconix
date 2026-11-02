#!/usr/bin/env python3
"""Loopback configuration checks, including AP/STA and non-Wi-Fi behavior."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parents[2]
opts = (root / "kernel/lwipopts.h").read_text()
net = (root / "kernel/net.c").read_text()
for name, value in {
    "LWIP_NETIF_LOOPBACK": "1",
    "LWIP_HAVE_LOOPIF": "1",
    "LWIP_NETIF_LOOPBACK_MULTITHREADING": "0",
    "LWIP_LOOPBACK_MAX_PBUFS": "8",
}.items():
    assert re.search(rf"^#define {name} {value}$", opts, re.M), name
assert 'loop_iface = netif_find("lo0");' in net
assert 'if (loop_iface) netif_poll(loop_iface);' in net
assert net.index('net_poll_active = 1;') < net.index('netif_poll(loop_iface);') < net.index('net_poll_active = 0;')
# Compile both branches of net_init() using existing host-mode regression.
subprocess.run(["python3", str(root / "tools/test_scripts/test_net_init_modes.py")], check=True)
subprocess.run(["python3", str(root / "tools/test_scripts/test_net_poll_nonwifi.py")], check=True)
print("PASS: loopback option and polling configuration")
