#!/usr/bin/env python3
"""Post-link audit of Pico W writable ELF allocations and fixed slot reserve.

This is a *link-time* check, not a measurement of runtime stack usage.
"""
import argparse
import re
import subprocess
import sys

RAM_START = 0x20000000
KERNEL_END = 0x20010000
SLOT_START = 0x20010000
RESERVE_START = 0x20038000
RAM_END = 0x20040000


def parse_sections(output):
    # readelf -SW: [Nr] Name Type Address Off Size ES Flg Lk Inf Al
    pattern = re.compile(r'^\s*\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\S+\s+(\S+)')
    for line in output.splitlines():
        match = pattern.match(line)
        if match:
            name, address, size, flags = match.groups()
            if 'A' in flags and 'W' in flags and int(size, 16):
                yield name, int(address, 16), int(size, 16)


def audit(sections):
    errors = []
    kernel_bytes = 0
    for name, start, size in sections:
        end = start + size
        if RAM_START <= start < RAM_END or RAM_START < end <= RAM_END:
            if not (RAM_START <= start < end <= KERNEL_END):
                errors.append(f'{name}: writable section 0x{start:08x}..0x{end-1:08x} outside 64-KiB kernel area')
            else:
                kernel_bytes += size
        elif start < RAM_END and end > RAM_START:
            errors.append(f'{name}: section crosses main SRAM boundary')
    return errors, kernel_bytes


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('elf')
    parser.add_argument('--readelf', default='arm-none-eabi-readelf')
    args = parser.parse_args()
    try:
        output = subprocess.check_output([args.readelf, '-SW', args.elf], text=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        sys.exit(f'ERROR: ELF section audit unavailable: {exc}')
    sections = list(parse_sections(output))
    if not sections:
        sys.exit('ERROR: no writable ELF sections found (cannot audit)')
    errors, kernel_bytes = audit(sections)
    for error in errors:
        print('ERROR:', error, file=sys.stderr)
    print(f'Pico W writable kernel sections: {kernel_bytes} bytes (sum, not peak RAM)')
    print(f'Kernel boundary: 0x{KERNEL_END:08x}; task slots: 0x{SLOT_START:08x}..0x{RESERVE_START-1:08x}')
    print(f'Wi-Fi reserve: 0x{RESERVE_START:08x}..0x{RAM_END-1:08x} (32 KiB)')
    print('NOTE: SDK scratch SRAM, runtime stacks, CYW43 allocations and lwIP heap require separate checks')
    if errors:
        sys.exit(1)
    print('PASS: writable ELF sections in main SRAM do not enter task or Wi-Fi reserve')

if __name__ == '__main__':
    main()
