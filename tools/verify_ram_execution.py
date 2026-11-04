#!/usr/bin/env python3
"""Inspect ELF program entry points and report whether they execute from SRAM."""
import argparse
import subprocess
import sys
p=argparse.ArgumentParser()
p.add_argument('elf')
p.add_argument('--nm', default='arm-none-eabi-nm')
a=p.parse_args()
try:
    out=subprocess.check_output([a.nm,'-n',a.elf],text=True)
except (OSError,subprocess.CalledProcessError) as exc:
    sys.exit(str(exc))
for name in ('main','blink_task','shell_task','os_svc_dispatch','isr_pendsv'):
    matches=[x.split() for x in out.splitlines() if x.split() and x.split()[-1]==name]
    if not matches:
        print(f'{name}: missing or local symbol not exported')
        continue
    addr=int(matches[-1][0],16)
    place='SRAM' if 0x20000000<=addr<0x20042000 else 'XIP/other'
    print(f'{name}: 0x{addr:08x} ({place})')
print('NOTE: SRAM addresses alone do not prove image loading and correct startup.')
