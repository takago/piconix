#!/usr/bin/env python3
"""Check selected kernel trace targets without changing runtime tables."""
from collections import Counter
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
kernel = (root / 'kernel/kernel.c').read_text()
cmake = (root / 'CMakeLists.txt').read_text()
block = kernel.split('#define TRACE_MATCH(fn)', 1)[1].split('#undef TRACE_MATCH', 1)[0]
names = re.findall(r'\bTRACE_MATCH\((\w+)\)\s*;', block)
assert len(names) == 51, f'expected 51 trace targets, found {len(names)}'
assert {'os_svc_dispatch', 'task_unblock'} <= set(names), 'missing scheduler/SVC targets'
assert 'tty_input_char' in names, 'missing UART/USB/Telnet shared TTY input target'
assert not ({'net_socket_tcp_accept_ready', 'net_socket_tcp_status', 'tty_rx_ready'} & set(names)), 'readiness probes must not be recorded'
assert len(set(names)) == len(names), f'duplicate targets: {[n for n,c in Counter(names).items() if c>1]}'

instrumented = set(re.findall(r'kernel/([\w]+\.c)', cmake.split('if(PICONIX_KERNEL_FUNCTION_TRACE)', 1)[1].split('APPEND PROPERTY COMPILE_OPTIONS -finstrument-functions', 1)[0]))
assert instrumented == {'kernel.c', 'adc.c', 'i2c.c', 'pwm.c', 'gpio.c', 'fb.c', 'net_socket.c', 'net_capture.c'}, instrumented

# A definition (not merely a call or prototype) must exist in an instrumented file.
# Internal cross-file names must also be declared in one of kernel's included headers.
headers = [root/'kernel'/h for h in ('devices.h', 'net_socket.h', 'net.h')]
header_text = '\n'.join(f.read_text() for f in headers)
locations = {}
for name in names:
    found = []
    pattern = re.compile(r'^[ \t]*(?:(?:static|inline|const|unsigned|signed|int|void|uint32_t|size_t|char|bool|long|short|struct|\w+_t)\s+)+\**' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', re.M)
    for file in sorted(instrumented):
        source = (root / 'kernel' / file).read_text()
        if pattern.search(source):
            found.append(file)
    assert len(found) == 1, f'{name}: expected one instrumented definition, got {found}'
    locations[name] = found[0]
    if found[0] != 'kernel.c':
        assert re.search(r'\b' + re.escape(name) + r'\s*\(', header_text), f'{name}: missing internal header declaration'

# The actual list still uses direct address matching and is guarded by trace-enable/context checks.
assert 'if(func==(void *)(uintptr_t)(fn)) name=#fn' in kernel
assert re.search(r'if\s*\(!trace_enabled\)\s*return\s*;', kernel)
assert 'trace_mode!=TRACE_MODE_ALL && ipsr!=11u' in kernel
# Polling and readiness probes are explicitly excluded from compiler instrumentation.
for filename, function_names in {
    'kernel.c': ('tty_rx_ready', 'wake_sockets', 'wake_selects', 'wake_readers', 'wake_waiters', 'wake_pipes', 'select_ready'),
    'net_socket.c': ('net_socket_tcp_accept_ready', 'net_socket_tcp_status', 'net_socket_tcp_accept_peek', 'net_socket_tcp_recv_ready', 'net_socket_tcp_write_ready', 'net_socket_tcp_listening', 'net_socket_udp_read_ready'),
}.items():
    source = (root / 'kernel' / filename).read_text()
    for function in function_names:
        assert re.search(r'\bNO_TRACE\s+(?:static\s+)?(?:int|void)\s+' + re.escape(function) + r'\s*\(', source), f'{filename}: {function} missing NO_TRACE'
assert 'TRACE_MATCH(tty_input_char)' in kernel
print('PASS: 51 unique targets, one instrumented definition each, declared cross-module entry points')

# Every definition in the eight instrumented source files must either be
# explicitly excluded or belong to the address-to-name trace target list.
# Include both conditional Pico and Pico W definitions in this source check.
import sys
sys.path.insert(0, str(root / 'tools'))
from gen_functions import c_functions
found_targets = set()
checked_definitions = 0
excluded_definitions = 0
for file in sorted(instrumented):
    source = (root / 'kernel' / file).read_text()
    lines = source.splitlines()
    assert '#define NO_TRACE __attribute__((no_instrument_function))' in source, file
    for name, line, *_ in c_functions(source):
        definition = lines[line - 1]
        if name == '__attribute__':
            assert 'no_instrument_function' in definition, (file, line)
            excluded_definitions += 1
        elif name in names:
            assert 'NO_TRACE' not in definition, (file, line, name)
            found_targets.add(name)
        else:
            assert re.match(r'^\s*NO_TRACE\b', definition), (file, line, name)
            excluded_definitions += 1
        checked_definitions += 1
assert found_targets == set(names), sorted(set(names) - found_targets)
print(f'PASS: {checked_definitions} function definitions classified, '
      f'{excluded_definitions} explicitly non-instrumented, '
      f'{len(found_targets)} unique trace targets')
