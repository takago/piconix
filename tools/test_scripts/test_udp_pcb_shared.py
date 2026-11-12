#!/usr/bin/env python3
"""UDP PCB lifecycle is shared by bind and first send without changing buffers."""
from pathlib import Path
import re

source = Path(__file__).resolve().parents[2] / "kernel/net_socket.c"
s = source.read_text()
body = s.split("#if PICONIX_WIFI", 1)[1].split("#else", 1)[0]
helper = re.search(r"static int udp_slot_open\(.*?\n}", body, re.S)
assert helper, "shared UDP setup missing"
h = helper.group(0)
for token in ("udp_new_ip_type", "udp_bind", "udp_remove", "udp_recv", "s->pcb = pcb", "s->rx_ready = 0"):
    assert token in h, token
assert body.count("udp_new_ip_type") == 1
assert "return udp_slot_open(s, port);" in body
assert "udp_slot_open(s, 0)" in body
assert "udp_slot_at(id) && udp_slot_at(id)" not in body
print("PASS: UDP PCB setup shared across bind and send")
