#!/usr/bin/env python3
"""Static regression guard for the Pico W polling service window."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
kernel = (root / 'kernel/kernel.c').read_text()
main = (root / 'kernel/main.c').read_text()
# 日本語コメントに依存せず，Wi-Fi用アイドル実行枠の条件と周期を検査する．
assert 'static uint32_t last_net_slice;' in kernel
assert '#if PICONIX_WIFI' in kernel
assert '(uint32_t)(ticks - last_net_slice) >= 50u' in kernel
assert 'next = IDLE_SLOT;' in kernel
assert 'net_poll();' in main
assert 'cyw43_arch_poll()' not in kernel
print('PASS: Pico W polling is scheduled in idle thread mode, not ISR')
