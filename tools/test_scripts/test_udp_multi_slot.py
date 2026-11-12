#!/usr/bin/env python3
"""Check fixed UDP socket slots, owner isolation and cleanup contracts."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
k=(ROOT/'kernel/kernel.c').read_text()
n=(ROOT/'kernel/net_socket.c').read_text()
h=(ROOT/'kernel/net_socket.h').read_text()
l=(ROOT/'kernel/lwipopts.h').read_text()
assert '#define NET_SOCKET_UDP_SLOTS 3' in h
assert 'static int udp_owner_pid[UDP_USER_SLOTS];' in k
assert 'udp_fd_slot(&tasks[current],a)' in k
assert 'net_socket_udp_recv(udp_fd_slot(t,fd)' in k
assert 'net_socket_udp_read_ready(id)' in k
assert 'net_socket_udp_close(id); udp_owner_pid[id]=0;' in k
assert 'static udp_slot_t udp_slots[NET_SOCKET_UDP_SLOTS];' in n
assert 'udp_recv(pcb, udp_received, s)' in n
assert 'udp_slot_t *s=(udp_slot_t *)arg;' in n
assert '#define MEMP_NUM_UDP_PCB 8' in l
assert 'u && u->pcb ? u->pcb->local_port : 0' in n
print('PASS: UDP slot ownership, receive isolation, cleanup and PCB capacity')
