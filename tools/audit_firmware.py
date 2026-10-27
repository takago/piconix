#!/usr/bin/env python3
"""Inspect the *actual* Pico SDK ELF; fail closed on unsafe RP2040 layouts.

This does not replace a real SDK build or a hardware test.
"""
import argparse
import re
import subprocess
import sys

SLOT_BASE = 0x20018000
SLOT_BYTES = 5 * 0x8000

def run(*argv):
    return subprocess.check_output(argv, text=True, stderr=subprocess.STDOUT)

def parse_nm(output):
    symbols = {}
    for line in output.splitlines():
        fields = line.split()
        if len(fields) >= 3 and re.fullmatch(r'[0-9a-fA-F]+', fields[0]):
            symbols[fields[2]] = int(fields[0], 16)
    return symbols

def parse_sections(output):
    result = {}
    # GNU objdump -h: 0 .boot2 00000100 10000000 10000000 ...
    for line in output.splitlines():
        match = re.match(r'^\s*\d+\s+(\S+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)', line)
        if match:
            name, size, vma, lma = match.groups()
            result[name] = tuple(int(x, 16) for x in (size, vma, lma))
    return result

def audit(symbols, sections):
    problems = []
    def require(cond, message):
        if not cond: problems.append(message)
    boot = sections.get('.boot2')
    require(boot is not None and boot[0] == 256 and boot[1] == 0x10000000,
            'Pico SDK .boot2 must occupy 256 bytes at 0x10000000')
    # The SDK copy-to-RAM linker script merges input .vectors into .flashtext.
    # Validate the symbol and its 48-entry footprint in a flash-backed section,
    # rather than assuming the input section survives as an output section.
    vector_address = symbols.get('__vectors', symbols.get('__VECTOR_TABLE'))
    if '__vectors' in symbols and '__VECTOR_TABLE' in symbols:
        require(symbols['__vectors'] == symbols['__VECTOR_TABLE'],
                'vector table symbol aliases disagree')
    require(vector_address == 0x10000100,
            'flash vector table must start at 0x10000100')
    vector_backing = any(size >= 192 and vma <= 0x10000100
                         and 0x10000100 + 192 <= vma + size
                         and lma == vma
                         for size, vma, lma in sections.values())
    require(vector_backing, 'flash vector table needs 192 flash-backed bytes')
    ram_vectors = sections.get('.ram_vector_table')
    require(ram_vectors is not None and ram_vectors[0] >= 192
            and ram_vectors[1] == 0x20000000,
            'RAM vector table must reserve >=192 bytes at 0x20000000')
    require(symbols.get('ram_vector_table') == 0x20000000,
            'ram_vector_table symbol must start at 0x20000000')
    base = symbols.get('task_ram')
    require(base == SLOT_BASE, 'task_ram must start at 0x20018000 (5 x 32-KiB slots)')
    for name in ('tasks', 'idle_stack'):
        address = symbols.get(name)
        require(address is not None, f'missing {name} symbol')
        if address is not None:
            require(not (SLOT_BASE <= address < SLOT_BASE + SLOT_BYTES),
                    f'{name} overlaps task memory')
    # Reject other allocatable SRAM sections intersecting task slots.
    # Ignore .bss.task_slots itself (it reserves the whole slot range).
    for name, (size, vma, _) in sections.items():
        if not size or name == '.bss.task_slots': continue
        if name.startswith('.debug') or name.startswith('.comment'): continue
        if max(vma, SLOT_BASE) < min(vma + size, SLOT_BASE + SLOT_BYTES):
            require(False, f'{name} overlaps reserved task slots: 0x{vma:08x}+0x{size:x}')
    # Check *both* embedded programs, not just Blink. In copy_to_ram mode
    # their .rodata may have an SRAM VMA and Flash LMA.
    for app in ('shell', 'blink'):
        image_start = symbols.get(f'piconix_{app}_image_start')
        image_end = symbols.get(f'piconix_{app}_image_end')
        require(image_start is not None and image_end is not None and image_end > image_start,
                f'missing/empty embedded {app} image')
        if image_start is None or image_end is None or image_end <= image_start:
            continue
        in_flash = 0x10000000 <= image_start < image_end <= 0x10200000
        in_kernel = 0x20000000 <= image_start < image_end <= 0x20018000
        require(in_flash or in_kernel,
                f'embedded {app} image must reside in XIP flash or kernel RAM')
        if in_kernel:
            containing = [(name, vma, lma, size) for name, (size, vma, lma)
                          in sections.items() if size and vma <= image_start
                          and image_end <= vma + size and 0x10000000 <= lma
                          and lma + size <= 0x10200000]
            require(bool(containing),
                    f'RAM-resident {app} image has no flash-backed load section')
    # Every allocated kernel SRAM section must fit within 96 KiB; no accidental
    # use of task banks by SDK sections, and no stack spill into the slots.
    for name, (size, vma, lma) in sections.items():
        if not size or name.startswith(('.debug', '.comment')):
            continue
        if 0x20000000 <= vma < 0x20018000:
            require(vma + size <= 0x20018000,
                    f'{name} crosses kernel 96-KiB boundary')
    return problems

def memory_report(sections):
    """Kernel high-water mark from allocated ELF SRAM sections (includes heap/stack)."""
    kernel_base, kernel_end = 0x20000000, SLOT_BASE
    ram_sections = [(name, size, addr) for name, (size, addr, _) in sections.items()
                    if size and kernel_base <= addr < kernel_end
                    and not name.startswith(('.debug', '.comment'))]
    used_end = max([kernel_base] + [addr + size for _, size, addr in ram_sections])
    used = max(0, used_end - kernel_base)
    capacity = kernel_end - kernel_base
    print('\n=== PicoNIX RP2040 SRAM layout ===')
    print('Region               Address range                Allocated / Capacity')
    print(f'Kernel (96 KiB)      0x{kernel_base:08x}..0x{kernel_end - 1:08x}     {used:6d} / {capacity:6d} bytes')
    print(f'Kernel headroom      (after allocated sections)   {max(0, capacity-used):6d} bytes')
    for i in range(5):
        base = SLOT_BASE + i * 0x8000
        print(f'User slot {i+1} (32 KiB)  0x{base:08x}..0x{base+0x7fff:08x}     reserved (runtime)')
    print('Trace ring            16384 bytes (inside kernel .bss)')
    print('Kernel section breakdown (allocated SRAM):')
    for name, size, addr in sorted(ram_sections, key=lambda x: x[2]):
        print(f'  {name:22s} 0x{addr:08x}  {size:6d} bytes')
    print('NOTE: headroom is linker-address headroom, not free heap or free task memory.')
    print('==================================\n')

def main():
    p = argparse.ArgumentParser()
    p.add_argument('elf')
    p.add_argument('--nm', default='arm-none-eabi-nm')
    p.add_argument('--objdump', default='arm-none-eabi-objdump')
    args = p.parse_args()
    try:
        symbols = parse_nm(run(args.nm, '-n', args.elf))
        sections = parse_sections(run(args.objdump, '-h', args.elf))
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit(f'FAIL: cannot inspect firmware ELF: {e}')
    problems = audit(symbols, sections)
    memory_report(sections)
    for name in ('.boot2', '.flashtext', '.ram_vector_table'):
        print(f'{name}: {sections.get(name, "MISSING")}')
    print(f'__vectors: {symbols.get("__vectors", "MISSING")}')
    print(f'task_ram: {symbols.get("task_ram", "MISSING")}')
    if problems:
        for problem in problems: print('FAIL:', problem)
        sys.exit(1)
    print('PASS: SDK boot2/flash+RAM vectors/slot/image static layout audit')
    print('NOTE: this does not prove RP2040 boot, correct vector handlers, or full-RAM execution')

if __name__ == '__main__': main()
