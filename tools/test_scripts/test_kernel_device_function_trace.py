#!/usr/bin/env python3
"""Verify optional instrumentation of device and socket kernel entry points."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
cm=(root/'CMakeLists.txt').read_text()
k=(root/'kernel/kernel.c').read_text()
for source in ('kernel/adc.c','kernel/i2c.c','kernel/pwm.c','kernel/gpio.c','kernel/fb.c','kernel/net_socket.c'):
    assert source in cm.split('APPEND PROPERTY COMPILE_OPTIONS -finstrument-functions')[0], source
for source, names in {
    'adc.c':('adc_driver_read','adc_driver_format'),
    'i2c.c':('i2c_driver_transfer',),
    'pwm.c':('pwm_driver_get','pwm_driver_set','pwm_parse_value'),
    'gpio.c':('gpio_driver_rw','gpio_driver_ioctl'),
    'fb.c':('fb_ioctl','fb_rw'),
    'net_socket.c':('net_socket_udp_bind','net_socket_udp_recv','net_socket_udp_send','net_socket_tcp_read','net_socket_tcp_write'),
}.items():
    src=(root/'kernel'/source).read_text()
    for name in names:
        assert f'TRACE_MATCH({name});' in k, name
        assert f'{name}(' in src, name
assert re.search(r'if\s*\(!trace_all_contexts\s*&&\s*ipsr!=11u\)\s*return\s*;', k)
assert '"Instrument selected kernel functions for trace" OFF' in cm
assert 'kernel/net_capture.c' in cm
for fn in ('net_echo_write','net_echo_read','net_echo_close'):
    assert f'TRACE_MATCH({fn});' in k
print('PASS: opted-in device, socket and ICMP trace coverage')
