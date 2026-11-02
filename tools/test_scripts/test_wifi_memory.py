#!/usr/bin/env python3
"""Static guards for the Pico W lwIP heap placement."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
opts = (root / 'kernel' / 'lwipopts.h').read_text()
cmake = (root / 'CMakeLists.txt').read_text()
header = (root / 'include/piconix.h').read_text()
assert re.search(r'#define\s+LWIP_RAM_HEAP_POINTER\s+\(\(void\s*\*\)0x2003C000u\)', opts)
assert re.search(r'#define\s+MEM_SIZE\s+\(10\s*\*\s*1024\)', opts)
assert '#define PBUF_POOL_SIZE 8' in opts
assert '#define LWIP_STATS 1' in opts and '#define MEM_STATS 1' in opts
assert '#define MEMP_STATS 1' in opts
assert '0x2003F000' in (root / 'kernel/kernel.c').read_text()
assert 'set(PICONIX_TASK_COUNT 5)' not in cmake  # Task slots are fixed by the current linker layout
assert 'pico_cyw43_arch_lwip_poll' in cmake
assert 'PICONIX_VERIFY_LAYOUT' in cmake
assert '0x2000E000' in (root/'kernel/main.c').read_text()
print('PASS: Pico W network bank has 10-KiB heap, 4-KiB idle stack and guarded separation')
print('NOTE: lwIP static pools remain in kernel RAM; measure /proc/net/memory on hardware')
