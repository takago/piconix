#!/usr/bin/env python3
"""Static contract for teaching packet capture; real Pico W hardware test required."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
k=(r/'kernel/kernel.c').read_text()
c=(r/'kernel/net_capture.c').read_text()
h=(r/'include/device_names.h').read_text()
cm=(r/'CMakeLists.txt').read_text()
help_text=(r/'userland/apps/help.c').read_text()
for name in ('packet','icmp'):
    assert f'/dev/net/{name}' in h
    assert f'userland/apps/{"tcpdump" if name=="packet" else "ping"}.c'
for name in ('ping','tcpdump'):
    assert (r/f'userland/apps/{name}.c').exists()
    assert f'APP STREQUAL "{name}"' not in cm  # use ordinary -Oz/PIC path
    assert f'"{name}"' in help_text
assert 'kernel/net_capture.c' in cm
assert 'net_capture_poll()' in (r/'kernel/net.c').read_text()
assert 'LWIP_HOOK_IP4_INPUT' in (r/'kernel/lwipopts.h').read_text()
assert 'net_packet_read(&tasks[current].fd_pos[fd]' in k
assert 'net_echo_write(' in k and 'net_echo_read(' in k
assert 'pbuf_copy_partial' in c and 'PACKET_COUNT 16' in c
for lang in ('en','ja'):
    for p in ['reference/network/NETWORK.md','reference/devices/DEVICES.md','guides/HELP.md']:
        assert ('tcpdump' in (r/f'docs/{lang}/{p}').read_text() or '/dev/net/packet' in (r/f'docs/{lang}/{p}').read_text())
print('PASS: capture/ICMP ABI, VFS, apps, and bilingual documentation')
