#!/usr/bin/env python3
"""Source-level guards for the STA-only diagnostics and idle stack."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
net = (root / "kernel/net.c").read_text()
kernel = (root / "kernel/kernel.c").read_text()
main = (root / "kernel/main.c").read_text()
header = (root / "include/piconix.h").read_text()
assert '#define IDLE_STACK_BYTES 4096u' in kernel
assert 'memset(idle_stack, 0xa5, sizeof(idle_stack));' in kernel
assert 'os_idle_stack_used_bytes(void)' in kernel
assert 'os_idle_stack_used_bytes(void)' in header
assert '#if PICONIX_WIFI_STA\n' in net
sta = net.split('#if PICONIX_WIFI_STA\n', 1)[1].split('#else\n', 1)[0]
assert 'cyw43_arch_enable_sta_mode();' in sta
assert 'cyw43_wifi_pm(&cyw43_state, CYW43_NONE_PM)' in sta
assert sta.index('cyw43_wifi_pm(&cyw43_state, CYW43_NONE_PM)') < sta.index('sta_begin_connect();', sta.index('cyw43_wifi_pm(&cyw43_state, CYW43_NONE_PM)'))
assert net.index('cyw43_arch_poll();', net.index('void net_poll(void) {')) < net.index('sta_poll();', net.index('void net_poll(void) {'))
assert 'cyw43_arch_enable_ap_mode(' in net
for key in ('PM_ERROR:', 'POLL_COUNT:', 'POLL_MAX_GAP_MS:', 'IDLE_STACK:'):
    assert key in net, key
assert 'net_poll();' in main
assert 'char text[448]; int len=0;' in kernel
assert not re.search(r'cyw43_wifi_pm\s*\([^;]*CYW43_NO_POWERSAVE_MODE', net)
print('PASS: STA power saving, idle stack and diagnostics guarded')
