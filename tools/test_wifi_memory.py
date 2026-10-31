#!/usr/bin/env python3
"""Static guards for the experimental Pico W lwIP heap placement."""
from pathlib import Path
import re
root = Path(__file__).resolve().parent.parent
opts = (root / 'kernel' / 'lwipopts.h').read_text()
cmake = (root / 'CMakeLists.txt').read_text()
header = (root / 'include/piconix.h').read_text()
assert re.search(r'#define\s+LWIP_RAM_HEAP_POINTER\s+\(\(void\s*\*\)0x20038000u\)', opts)
assert re.search(r'#define\s+MEM_SIZE\s+\(4\s*\*\s*1024\)', opts)
assert 'set(PICONIX_TASK_COUNT 5)' in cmake
assert 'pico_cyw43_arch_lwip_poll' in cmake
assert 'PICONIX_VERIFY_LAYOUT' in cmake
assert '0x20010000' in header or '0x20010000' in (root/'kernel/main.c').read_text()
print('PASS: Pico W lwIP heap starts in the dedicated network reserve (4 KiB of 32 KiB)')
print('NOTE: CYW43/lwIP static allocations and stack usage remain unmeasured')
