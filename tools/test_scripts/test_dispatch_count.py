#!/usr/bin/env python3
"""Verify per-task dispatch accounting is distinct from context switches."""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[2]
source=(root/'kernel/kernel.c').read_text()
top=(root/'userland/apps/top.c').read_text()
assert 'uint32_t dispatch_count;' in source
assert 'volatile uint32_t cpu_ticks;' in source
assert 'tasks[IDLE_SLOT].dispatch_count' in source
assert 't->dispatch_count=0;' in source
schedule=source.split('NO_TRACE uint32_t *os_schedule(uint32_t *oldsp)',1)[1].split('NO_TRACE void os_start(',1)[0]
assert re.search(r'(?m)^\s*tasks\[next\]\.dispatch_count\+\+;',schedule)
assert 'if(previous!=next)' not in schedule.split('tasks[next].dispatch_count++;')[0]
assert re.search(r'if\s*\(previous\s*>=\s*0\s*&&\s*previous\s*!=\s*next\)\s*\{\s*context_switch_count\+\+;',schedule)
assert 'PPID  DISPATCH  CPU_TICKS   NET_US   CON_US\\n' in source
assert '(unsigned long)t->dispatch_count' in source
assert 'DISPATCH PPID' in top
assert 'SWITCHES PPID' not in top
print('PASS: dispatch counting includes unchanged selections and preserves context-switch accounting')
