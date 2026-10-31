#!/usr/bin/env python3
"""Static regression guard for the Pico W polling service window."""
from pathlib import Path
root = Path(__file__).resolve().parent.parent
kernel = (root / 'kernel/kernel.c').read_text()
main = (root / 'kernel/main.c').read_text()
assert '#if PICONIX_WIFI\n    /* Give the polling lwIP/CYW43' in kernel
assert '(uint32_t)(ticks - last_net_slice) >= 50u' in kernel
assert 'next = IDLE_SLOT;' in kernel
assert 'net_poll();' in main
assert 'cyw43_arch_poll()' not in kernel
print('PASS: Pico W polling is scheduled in idle thread mode, not ISR')
