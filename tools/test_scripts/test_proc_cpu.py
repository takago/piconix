#!/usr/bin/env python3
"""Check the /proc/cpu VFS contract and lightweight time accounting hooks."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
kernel = (root / 'kernel/kernel.c').read_text()
main = (root / 'kernel/main.c').read_text()
header = (root / 'include/piconix.h').read_text()

assert 'DEV_PROC_CPU = 147' in kernel
assert '{"/proc/cpu",DEV_PROC_CPU}' in kernel
assert 'dev==DEV_PROC_CPU ||' in kernel
assert 'EMIT("cpu",0)' in kernel
assert 'if(dev==DEV_PROC_CPU)' in kernel
for name in ('CPU Usage (%)', 'CPU Time (ms)', 'CPU_HISTORY 30',
             'cpu_history_serial', 'proc_cpu_read', 'pct'):
    assert name in kernel
assert 'if(ticks%1000u==0u)' in kernel
assert 'current>=0 && current!=IDLE_SLOT' in kernel
assert 'cpu_net_total_us-cpu_last_net_us' in kernel
assert 'cpu_console_total_us-cpu_last_console_us' in kernel
assert 'cpu_history_serial=0' in kernel
for name in ('net', 'console'):
    assert 'os_cpu_account_'+name in header
    assert 'os_cpu_account_'+name in main
    assert 'NO_TRACE void os_cpu_account_'+name in kernel
assert 'time_us_32()' in main
assert 'add_repeating_timer_us(-250' in main
assert 'if(idle_network_timer_ready) __wfi();' in main
for lang in ('ja', 'en'):
    assert '/proc/cpu' in (root / f'docs/{lang}/reference/CPU.md').read_text()
print('PASS: /proc/cpu registration, 1s snapshot, counters, docs')
