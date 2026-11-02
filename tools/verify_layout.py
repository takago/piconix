#!/usr/bin/env python3
"""Fail closed when PicoNIX' reserved SRAM layout cannot be verified.

Checks ELF symbols (VMA), not the .bin size. This does not prove RAM execution.
"""
import argparse
import subprocess
import sys

MAIN_START = 0x20000000
MAIN_END = 0x20040000
SLOT_SIZE = 0x6000


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
    ap.add_argument('--slots', type=int, choices=(6,), default=6)
    ap.add_argument('--nm', default='arm-none-eabi-nm')
    ap.add_argument('--wifi', action='store_true', help='Pico W idle stack is in network SRAM')
    args = ap.parse_args()
    try:
        sym = symbols(args.elf, args.nm)
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit(f'ERROR: cannot inspect ELF: {e}')
    start = sym.get('task_ram')
    if start is None:
        sys.exit('ERROR: task_ram symbol not found; cannot verify layout')
    end = start + args.slots * SLOT_SIZE
    print(f'task_ram: 0x{start:08x}..0x{end-1:08x} ({args.slots} x 24 KiB)')
    if start != 0x2000e000:
        sys.exit('ERROR: fixed image requires task_ram at 0x2000e000 ; SDK default linker may not satisfy this')
    if start < MAIN_START or end > MAIN_END:
        sys.exit('ERROR: task slots do not fit into 256-KiB main SRAM')
    for key in ('tasks', 'idle_stack'):
        addr = sym.get(key)
        if addr is None:
            sys.exit(f'ERROR: missing {key} symbol')
        if start <= addr < end:
            sys.exit(f'ERROR: {key} overlaps task slots')
        if key == 'idle_stack':
            if args.wifi and addr != 0x2003F000:
                sys.exit('ERROR: Pico W idle stack must start at 0x2003f000')
            if not args.wifi and not MAIN_START <= addr < start:
                sys.exit('ERROR: ordinary Pico idle stack must be in kernel SRAM')
        print(f'{key}: 0x{addr:08x}')
    for name, address in (('shared_ram',0x20032000),('ramfile_data',0x20032800)):
        if sym.get(name) != address:
            sys.exit(f'ERROR: {name} must be at 0x{address:08x}')
    if args.slots == 6:
        if sym.get('trace_ring') != 0x20038000:
            sys.exit('ERROR: trace_ring must be located at 0x20038000')
        reserve_start = 0x2003C000
        reserve_end = MAIN_END
        if end != 0x20032000:
            sys.exit('ERROR: task slots must end at SHM start')
        print('SHM: 0x20032000..0x200327ff (2 KiB)')
        print('RAM files: 0x20032800..0x20037fff (11 x 2 KiB)')
        print('Trace ring: 0x20038000..0x2003bfff (16 KiB)')
        # The lwIP heap is an absolute address, not an ELF symbol. Verify the
        # intended reservation and heap definition together, before trusting it.
        from pathlib import Path
        import re
        opts = (Path(__file__).resolve().parent.parent / 'kernel' / 'lwipopts.h').read_text()
        heap = re.search(r'^#define\s+MEM_SIZE\s+\(?(\d+)\s*\*\s*1024\)?', opts, re.M)
        ptr = re.search(r'^#define\s+LWIP_RAM_HEAP_POINTER\s+\(\(void\s*\*\)\s*(0x[0-9a-fA-F]+)u?\)', opts, re.M)
        if not heap or not ptr:
            sys.exit('ERROR: cannot verify Wi-Fi lwIP heap size and location')
        heap_size = int(heap.group(1)) * 1024
        heap_start = int(ptr.group(1), 16)
        # lwIP needs at least one aligned heap header beyond MEM_SIZE.
        # Keep a full 2-KiB gap before the dedicated idle stack.
        idle_start = 0x2003F000
        if heap_start != reserve_start or heap_start + heap_size > idle_start - 2048:
            sys.exit('ERROR: lwIP heap/idle stack boundary gap is too small')
        print(f'Wi-Fi reserve: 0x{reserve_start:08x}..0x{reserve_end-1:08x} (16 KiB)')
        print(f'lwIP heap payload: 0x{heap_start:08x}..0x{heap_start+heap_size-1:08x} ({heap_size} bytes)')
        if args.wifi:
            print('Idle stack: 0x2003f000..0x2003ffff (4096 bytes)')
            print('Gap for lwIP allocator metadata: 0x2003e800..0x2003efff (2048 bytes)')
        print('NOTE: CYW43 and lwIP static RAM still consume kernel RAM; verify map and runtime usage')
    print('PASS: fixed physical task slots and selected kernel objects are nonoverlapping')
    print('NOTE: this does NOT establish task-local code/data or full-RAM execution')


if __name__ == '__main__':
    main()
