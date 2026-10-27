#!/usr/bin/env python3
"""Fail closed when PicoNIX' reserved SRAM layout cannot be verified.

Checks ELF symbols (VMA), not the .bin size. This does not prove RAM execution.
"""
import argparse
import subprocess
import sys

MAIN_START = 0x20000000
MAIN_END = 0x20040000
SLOT_SIZE = 0x8000


def symbols(elf, nm):
    output = subprocess.check_output([nm, '-n', elf], text=True)
    result = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 3:
            try:
                result[fields[2]] = int(fields[0], 16)
            except ValueError:
                pass
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('elf')
    ap.add_argument('--nm', default='arm-none-eabi-nm')
    args = ap.parse_args()
    try:
        sym = symbols(args.elf, args.nm)
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit(f'ERROR: cannot inspect ELF: {e}')
    start = sym.get('task_ram')
    if start is None:
        sys.exit('ERROR: task_ram symbol not found; cannot verify layout')
    end = start + 6 * SLOT_SIZE
    print(f'task_ram: 0x{start:08x}..0x{end-1:08x} (6 x 32 KiB)')
    if start != 0x20010000:
        sys.exit('ERROR: fixed image requires task_ram at 0x20010000 ; SDK default linker may not satisfy this')
    if start % SLOT_SIZE:
        sys.exit('ERROR: task_ram is not 32-KiB aligned')
    if start < MAIN_START or end > MAIN_END:
        sys.exit('ERROR: task slots do not fit into 256-KiB main SRAM')
    for key in ('tasks', 'idle_stack'):
        addr = sym.get(key)
        if addr is None:
            sys.exit(f'ERROR: missing {key} symbol')
        if start <= addr < end:
            sys.exit(f'ERROR: {key} overlaps task slots')
        print(f'{key}: 0x{addr:08x}')
    print('PASS: fixed physical task slots and selected kernel objects are nonoverlapping')
    print('NOTE: this does NOT establish task-local code/data or full-RAM execution')


if __name__ == '__main__':
    main()
