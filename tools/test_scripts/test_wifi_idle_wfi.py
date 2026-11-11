#!/usr/bin/env python3
"""Guard the optional 250-us idle wakeup; Wi-Fi must stay in thread mode."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
main = (root / 'kernel/main.c').read_text()
net = (root / 'kernel/net.c').read_text()
assert 'add_repeating_timer_us(-250, idle_network_wakeup,' in main
assert 'idle_network_timer_ready=add_repeating_timer_us(' in main
assert 'if(idle_network_timer_ready) __wfi();' in main
assert '#else\n        __wfi();' in main
callback = main.split('static bool idle_network_wakeup(', 1)[1].split('\n}', 1)[0]
assert 'return true;' in callback
for prohibited in ('net_poll(', 'cyw43_', 'os_console_poll(', 'lwip_'):
    assert prohibited not in callback
assert main.index('net_poll();') < main.index('os_console_poll();') < main.index('if(idle_network_timer_ready) __wfi();')
assert '#define NET_POLL_INTERVAL_US 250ull' in net
print('PASS: 250us idle IRQ wakeup, network processing outside IRQ, fallback busy poll')
