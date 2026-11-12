#!/usr/bin/env python3
"""Source-level guards for state cleanup; the linker must validate actual SRAM use."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
s = (root / 'kernel/kernel.c').read_text()
assert 'startup_seconds' not in s and 'script_path' not in s
assert 'uint16_t adc_values[OS_FD_MAX]' in s
assert 'dst->adc_values[to] = src->adc_values[from];' in s
assert 'if(tasks[current].fd_pos[fd]==0)' in s
assert 'uint16_t size, zero_start, zero_size;' in s
for field in ('size', 'zero_start', 'zero_size'):
    assert f'images[id].{field} = (uint16_t){field};' in s
assert 'size > OS_TASK_BYTES - 4096u' in s
assert 'zero_size > OS_TASK_BYTES - 4096u - zero_start' in s
assert '(zero_size && zero_start < size)' in s
assert 'OS_MAX_TASKS == 6 && OS_TASK_BYTES == 24576u' in s
print('PASS: kernel task state, RAM image bounds and ADC FD cache')
