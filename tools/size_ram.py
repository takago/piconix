#!/usr/bin/env python3
"""PicoNIX firmware and task memory report (KiB).

Run from the repository root: python3 tools/size_ram.py
No Python dependencies; requires readelf (or arm-none-eabi-readelf).
The firmware .map is used for approximate per-component attribution.
"""
import argparse
from collections import defaultdict
from pathlib import Path
import re
import shutil
import subprocess
import sys

SLOT_SIZE = 24 * 1024
STACK_SIZE = 4 * 1024
FLASH_START, FLASH_END = 0x10000000, 0x11000000
SRAM_START, SRAM_END = 0x20000000, 0x20042000
CATEGORIES = ('PicoNIX', 'Embedded apps', 'Symbol catalogs', 'Pico SDK', 'lwIP', 'CYW43', 'Other')
KERNEL_START, KERNEL_END = 0x20000000, 0x2000E000
RAM_LAYOUT = (
    ('Kernel',    0x20000000, 0x2000E000),
    ('User slots (6 x 24)', 0x2000E000, 0x20032000),
    ('Shared memory', 0x20032000, 0x20032800),
    ('RAM files', 0x20032800, 0x20038000),
    ('Trace', 0x20038000, 0x2003C000),
    ('Network reserve', 0x2003C000, 0x20040000),
)


def fmt(n):
    return f'{n / 1024:8.2f}'


def command_output(exe, *args):
    p = subprocess.run([exe, *map(str, args)], text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if p.returncode:
        raise RuntimeError(f'{exe}: {p.stderr.strip()}')
    return p.stdout


def tool(prefix):
    return shutil.which('arm-none-eabi-' + prefix) or shutil.which(prefix)


# readelf -W -S, including section names, sizes and VMAs.
SEC_RE = re.compile(r'^\s*\[\s*\d+\]\s+(\S+)\s+\S+\s+([0-9a-fA-F]+)\s+[0-9a-fA-F]+\s+([0-9a-fA-F]+)\s+\S+\s+([WAXMSILOGTE]*)\b')
SYM_RE = re.compile(r'^\s*\d+:\s*([0-9a-fA-F]+)\s+\d+\s+\S+\s+\S+\s+\S+\s+\S+\s+(\S+)')


def elf_info(elf, readelf):
    sections = []
    for line in command_output(readelf, '-W', '-S', elf).splitlines():
        m = SEC_RE.match(line)
        if m:
            name, addr, size, flags = m.groups()
            if 'A' in flags:
                sections.append((name, int(addr, 16), int(size, 16)))
    syms = {}
    for line in command_output(readelf, '-W', '-s', elf).splitlines():
        m = SYM_RE.match(line)
        if m:
            syms[m.group(2)] = int(m.group(1), 16)
    return sections, syms


def app_rows(build, readelf):
    rows, warnings = [], []
    for elf in sorted(build.glob('ram_*/*.elf')):
        if elf.stem.endswith('_alt'):
            continue
        try:
            sections, syms = elf_info(elf, readelf)
        except RuntimeError as exc:
            warnings.append(f'{elf}: {exc}')
            continue
        code = sum(s for n, a, s in sections if n == '.text' or n.startswith('.rodata'))
        data = sum(s for n, a, s in sections if n == '.data')
        bss = sum(s for n, a, s in sections if n == '.bss')
        end = syms.get('__bss_end')
        stack_bottom = syms.get('__stack_bottom')
        if end is None or stack_bottom is None:
            warnings.append(f'{elf}: __bss_end / __stack_bottom not found (skipped)')
            continue
        free = stack_bottom - end
        used = SLOT_SIZE - free
        rows.append((elf.stem, code, data, bss, STACK_SIZE, used, free))
    rows.sort(key=lambda r: (-r[5], r[0]))
    return rows, warnings


def component(source):
    """Classify *input* object paths; this is not a symbol-level classifier."""
    s = source.replace('\\', '/').lower()
    if 'embed_kernel_symbols' in s or 'embed_symbols' in s:
        return 'Symbol catalogs'
    if 'embed_image' in s or re.search(r'/ram_[^/]+/', s):
        return 'Embedded apps'
    if 'lwip' in s:
        return 'lwIP'
    if 'cyw43' in s:
        return 'CYW43'
    if re.search(r'(^|/)(kernel|userland)/', s) or 'initial_fs' in s:
        return 'PicoNIX'
    if any(x in s for x in ('pico-sdk', '/src/rp2_common/', '/src/rp2040/',
                            '/src/common/', '/src/boards/', '/hardware_',
                            '/pico_', 'tinyusb', 'mbedtls', 'btstack')):
        return 'Pico SDK'
    if re.search(r'(^|/)cmakefiles/piconix\.dir/', s):
        return 'PicoNIX'
    return 'Other'


# GNU ld map input sections, which may be split across two lines.
OUTPUT_RE = re.compile(r'^(\.[\w.$+-]+)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\b')
INPUT_RE = re.compile(r'^\s+(\.[\w.$+-]+|COMMON)\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(.+?)\s*$')
CONT_RE = re.compile(r'^\s+0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+)\s+(.+?)\s*$')
OBJECT_RE = re.compile(r'(?:[^\s]+\.a\([^)]*\.o\)|[^\s]+\.(?:o|obj))(?:\([^)]*\))?$')
APP_SYM_RE = re.compile(r'^piconix_(.+)_image_(start|end)$')


def overlap(start, end, lo, hi):
    return max(0, min(end, hi) - max(start, lo))


def interval_union(intervals):
    """Count only distinct addressed bytes, avoiding overlap/double counting."""
    length = 0
    previous = None
    for a, b in sorted(intervals):
        if previous is None:
            previous = [a, b]
        elif a <= previous[1]:
            previous[1] = max(previous[1], b)
        else:
            length += previous[1] - previous[0]
            previous = [a, b]
    if previous is not None:
        length += previous[1] - previous[0]
    return length


def memory_kind(addr):
    if FLASH_START <= addr < FLASH_END:
        return 'Flash'
    if SRAM_START <= addr < SRAM_END:
        return 'SRAM'
    return None


def allocated_ranges(sections):
    return [(a, a + size, memory_kind(a)) for name, a, size in sections
            if size > 0 and memory_kind(a) is not None]


def embedded_ranges(syms, sections):
    """Get real embedded PIC byte intervals from the firmware ELF symbol pairs.

    These come from embed_image.S.in, not from map input section naming.
    Only allocated Flash bytes count; padding after each image is excluded.
    """
    endpoints = defaultdict(dict)
    for name, addr in syms.items():
        m = APP_SYM_RE.fullmatch(name)
        if m:
            endpoints[m.group(1)][m.group(2)] = addr
    flash_ranges = [(a, a + n) for _, a, n in sections
                    if n > 0 and memory_kind(a) == 'Flash']
    images = []
    for name, endpoints_pair in sorted(endpoints.items()):
        start = endpoints_pair.get('start')
        end = endpoints_pair.get('end')
        if start is None or end is None or end <= start:
            continue
        # Intersect with allocated ELF Flash regions and avoid double counting.
        for lo, hi in flash_ranges:
            a, b = max(start, lo), min(end, hi)
            if a < b:
                images.append((a, b, 'Flash', 'Embedded apps'))
    return images


def map_report(path, sections, embedded=()):
    """Attribute only bytes that fall inside an ELF allocated output section.

    Lines in discarded/non-allocated map sections are ignored. For unusual
    overlapping records, an embedded-image ELF symbol span takes priority;
    otherwise the first category wins. The remainder is unattributed.
    """
    elf_ranges = allocated_ranges(sections)
    ranges = []
    pending = False
    current_output = None
    for line in path.read_text(errors='replace').splitlines():
        m_out = OUTPUT_RE.match(line)
        if m_out:
            current_output = m_out.group(1)
            pending = False
            continue
        if line.lstrip().startswith('*fill*') or line.lstrip().startswith('LOAD '):
            pending = False
            continue
        m = INPUT_RE.match(line)
        if m:
            name, addr, sz, src = m.groups()
            pending = False
        else:
            m = CONT_RE.match(line) if pending else None
            if m:
                addr, sz, src = m.groups()
            else:
                pending = bool(re.match(r'^\s+\.(?:text|rodata|data|bss|ram|time_critical|binary_info|uninitialized_data)[\w.$+-]*\s*$', line))
                continue
        pending = False
        if current_output is None or not OBJECT_RE.search(src):
            continue
        start, size = int(addr, 16), int(sz, 16)
        if size <= 0:
            continue
        end = start + size
        for lo, hi, kind in elf_ranges:
            a, b = max(lo, start), min(hi, end)
            if b > a:
                ranges.append((a, b, kind, component(src)))
    # Symbol intervals have priority over path-based map ownership; this
    # prevents an embedded image also counting as PicoNIX or unattributed.
    ranges.extend(embedded)
    # Assign an individual address at most once, even if the map has aliases.
    counters = defaultdict(int)
    for kind in ('Flash', 'SRAM'):
        events = []
        for a, b, typ, cat in ranges:
            if typ == kind:
                events.append((a, 1, cat))
                events.append((b, -1, cat))
        events.sort()
        active = defaultdict(int)
        last = None
        for x, direction, category in events:
            if last is not None and x > last and any(v > 0 for v in active.values()):
                owners = [c for c in CATEGORIES if active[c] > 0]
                chosen = 'Embedded apps' if active['Embedded apps'] > 0 else owners[0]
                counters[(chosen, kind)] += x - last
            active[category] += direction
            last = x
    return counters


def sram_region_totals(sections):
    """Return unique allocated ELF SRAM bytes, partitioned by fixed regions."""
    ranges = [(a, a + sz) for _, a, sz in sections
              if sz > 0 and memory_kind(a) == 'SRAM']
    totals = []
    for name, start, end in RAM_LAYOUT:
        inside = [(max(a, start), min(b, end)) for a, b in ranges
                  if min(b, end) > max(a, start)]
        totals.append((name, interval_union(inside)))
    # Anything in the RP2040 scratch SRAM or outside documented reserved regions.
    covered = [(start, end) for _, start, end in RAM_LAYOUT]
    cuts = sorted({x for pair in covered for x in pair} |
                  {SRAM_START, SRAM_END})
    outside = 0
    for lo, hi in zip(cuts, cuts[1:]):
        if not any(start <= lo and hi <= end for start, end in covered):
            outside += interval_union([(max(a, lo), min(b, hi))
                                       for a, b in ranges if min(b, hi) > max(a, lo)])
    return totals, outside


def print_fixed_layout(sections):
    print('\n=== Fixed SRAM map (KiB, RP2040) ===')
    print(f"{'Region':23} {'Start':>11} {'End excl.':>11} {'Reserved':>9} {'ELF bytes':>10}")
    print('-' * 71)
    region_totals, outside = sram_region_totals(sections)
    for (name, start, end), (_, elf_in_region) in zip(RAM_LAYOUT, region_totals):
        print(f'{name:23} 0x{start:08x} 0x{end:08x} {fmt(end - start)} {fmt(elf_in_region)}')
    print('  Reserved is a fixed allocation, not additional ELF consumption.')
    print('  Network: 10 KiB lwIP heap + 2 KiB gap + 4 KiB idle stack (Pico W).')
    print(f'  Other ELF SRAM (including scratch banks): {fmt(outside).strip()} KiB')
    region_sum = sum(v for _, v in region_totals) + outside
    total = interval_union([(a, a + sz) for _, a, sz in sections
                            if sz and memory_kind(a) == 'SRAM'])
    print(f'  Reconciliation: regions + other = {fmt(region_sum).strip()} KiB; '
          f'ELF SRAM = {fmt(total).strip()} KiB')
    if region_sum != total:
        print('  WARNING: SRAM regions do not reconcile; check ELF addresses/layout.')
    print('  RP2040 also has 8 KiB scratch SRAM (0x20040000..0x20041fff).')


def kernel_stack_bounds(syms):
    # Only claim a stack reservation when the linker defines both boundaries.
    candidates = [('__StackLimit', '__StackTop'), ('__stack_bottom', '__stack_top'),
                  ('__StackBottom', '__StackTop')]
    for bottom, top in candidates:
        lo, hi = syms.get(bottom), syms.get(top)
        if lo is not None and hi is not None and KERNEL_START <= lo < hi <= KERNEL_END:
            return lo, hi
    return None


def print_kernel(build, readelf, map_path):
    elf = build / 'piconix.elf'
    if not elf.exists():
        print(f'Firmware ELF not found: {elf}')
        return
    sections, syms = elf_info(elf, readelf)
    flash_ranges = [(a, a + n) for _, a, n in sections if n and memory_kind(a) == 'Flash']
    ram_ranges = [(a, a + n) for _, a, n in sections if n and memory_kind(a) == 'SRAM']
    flash = interval_union(flash_ranges)
    sram = interval_union(ram_ranges)
    kernel_intervals = [(max(a, KERNEL_START), min(b, KERNEL_END)) for a, b in ram_ranges
                        if min(b, KERNEL_END) > max(a, KERNEL_START)]
    kernel_allocated = interval_union(kernel_intervals)
    data = sum(n for name, a, n in sections if name.startswith('.data') and
               KERNEL_START <= a < KERNEL_END)
    bss = sum(n for name, a, n in sections if name.startswith('.bss') and
              KERNEL_START <= a < KERNEL_END)
    stack = kernel_stack_bounds(syms)
    stack_bytes = stack[1] - stack[0] if stack else None
    reserved = kernel_intervals[:]
    if stack:
        reserved.append(stack)
    used = interval_union(reserved)
    free = (KERNEL_END - KERNEL_START) - used
    print('=== Firmware (KiB; entire linked image) ===')
    print(f"{'Component':16} {'Code':>8} {'Data':>8} {'BSS':>8} {'Stack':>8} {'Used':>8} {'Free':>8}")
    print('-' * 76)
    stack_text = fmt(stack_bytes) if stack_bytes is not None else '       -'
    print(f"{'Firmware':16} {fmt(flash)} {fmt(data)} {fmt(bss)} {stack_text} {fmt(used)} {fmt(free)}")
    print('  Code = all allocated firmware Flash (PicoNIX + SDK + lwIP + CYW43 + images).')
    print('  Used/Free = allocated SRAM within the shared 56 KiB kernel RAM region.')
    if not stack:
        print('  WARNING: Free is an upper bound; unknown stack/runtime allocations not subtracted.')
    kernel_other = sram - kernel_allocated
    print(f'  Firmware total: Flash {fmt(flash).strip()} KiB, '
          f'ELF SRAM (all regions) {fmt(sram).strip()} KiB')
    print(f'  SRAM split: kernel region {fmt(kernel_allocated).strip()} KiB; '
          f'outside kernel region {fmt(kernel_other).strip()} KiB')
    if kernel_other:
        print('  NOTE: Outside-kernel ELF sections are excluded from the 56 KiB Free calculation.')
    print('  Data/BSS = sections inside kernel region; Used also includes other allocated kernel-region sections.')
    if not map_path.exists():
        print(f'  Link map not found: {map_path}')
        print('  Kernel-only section attribution unavailable without the linker map.')
        print_fixed_layout(sections)
        return
    images = embedded_ranges(syms, sections)
    buckets = map_report(map_path, sections, images)
    # Attribute PicoNIX-only bytes by intersecting the same linker-map inputs
    # with the actual allocated .data/.bss ELF sections.  Do not mix firmware
    # totals with the kernel component, which shares the 56 KiB SRAM region.
    data_sections = [sec for sec in sections if sec[0].startswith('.data')]
    bss_sections = [sec for sec in sections if sec[0].startswith('.bss')]
    by_data = map_report(map_path, data_sections)
    by_bss = map_report(map_path, bss_sections)
    kernel_code = buckets['PicoNIX', 'Flash']
    kernel_data = by_data['PicoNIX', 'SRAM']
    kernel_bss = by_bss['PicoNIX', 'SRAM']
    kernel_sram = buckets['PicoNIX', 'SRAM']
    print('\n=== PicoNIX kernel component (KiB; linker-map attribution) ===')
    print(f"{'Component':16} {'Code':>8} {'Data':>8} {'BSS':>8} {'Stack':>8} {'Used':>8} {'Free':>8}")
    print('-' * 76)
    print(f"{'Kernel':16} {fmt(kernel_code)} {fmt(kernel_data)} {fmt(kernel_bss)} {'-':>8} {fmt(kernel_sram)} {'-':>8}")
    print('  Code = PicoNIX-attributed allocated Flash, including constants.')
    print('  Used = PicoNIX-attributed ELF SRAM, including other allocated RAM sections.')
    print('  Stack/Free = not separately attributable; PicoNIX, SDK, lwIP and CYW43')
    print('  share the 56 KiB kernel RAM region. See Firmware above for region headroom.')
    print('  Attribution is based on linker-map input object paths, not individual symbols.')
    total_flash = sum(buckets[c, 'Flash'] for c in CATEGORIES)
    total_sram = sum(buckets[c, 'SRAM'] for c in CATEGORIES)
    print('\n=== Link-map attribution (KiB; allocated ELF addresses only) ===')
    print(f"{'Component':19} {'Flash':>9} {'SRAM':>9}")
    print('-' * 39)
    for cat in CATEGORIES:
        print(f'{cat:19} {fmt(buckets[cat, "Flash"])} {fmt(buckets[cat, "SRAM"])}')
    print('-' * 39)
    print(f'{"Attributed objects":19} {fmt(total_flash)} {fmt(total_sram)}')
    print(f'{"Unattributed / padding":19} {fmt(max(0, flash-total_flash))} {fmt(max(0, sram-total_sram))}')
    print(f'{"ELF allocated":19} {fmt(flash)} {fmt(sram)}')
    print('  SRAM column is for the ENTIRE ELF, including any sections outside kernel RAM.')
    print('  It is not the same measure as the 56 KiB kernel-region Used/Free above.')
    print('  Entries are address-clipped and deduplicated; archive/object paths estimate ownership.')
    print('  Embedded apps use ELF image_start/end symbols; their bytes take precedence over map paths.')
    if not images:
        print('  WARNING: No embedded app image symbol pairs found in firmware ELF.')
    else:
        print(f'  Embedded app images: {len(set((a, b) for a, b, _, _ in images))} allocated intervals.')
    print('  Symbol catalogs are classified from linker map object paths.')
    print('  .data Flash load copies, if any, are not added to VMA Flash accounting.')
    print('  Live lwIP heap/pbuf/PCB metrics: cat /proc/net/memory')
    print_fixed_layout(sections)


def main():
    parser = argparse.ArgumentParser(description='PicoNIX kernel and task memory (KiB)')
    parser.add_argument('--build', type=Path, default=Path('build-pico_w'),
                        help='build folder (default: build-pico_w)')
    parser.add_argument('--map', dest='map_path', type=Path, default=None,
                        help='firmware GNU ld map path (autodetect if omitted)')
    args = parser.parse_args()
    readelf = tool('readelf')
    if not readelf:
        parser.error('readelf not found; install binutils / gcc-arm-none-eabi')
    build = args.build
    map_path = args.map_path
    if map_path is None:
        candidates = [build / 'piconix.elf.map', build / 'piconix.map']
        candidates += sorted(build.glob('*.map'))
        map_path = next((p for p in candidates if p.is_file()), candidates[0])
    try:
        print_kernel(build, readelf, map_path)
        rows, warnings = app_rows(build, readelf)
    except (RuntimeError, OSError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        return 1
    print('\n=== User applications (KiB, 24 KiB per slot) ===')
    print(f"{'Program':14} {'Code':>8} {'Data':>8} {'BSS':>8} {'Stack':>8} {'Used':>8} {'Free':>8}")
    print('-' * 74)
    for name, code, data, bss, stack, used, free in rows:
        print(f'{name:14} {fmt(code)} {fmt(data)} {fmt(bss)} {fmt(stack)} {fmt(used)} {fmt(free)}')
    print(f'Applications: {len(rows)}; fixed slot={SLOT_SIZE // 1024} KiB, reserved stack={STACK_SIZE // 1024} KiB')
    print('  Free = __stack_bottom - __bss_end (static headroom, NOT unused runtime stack).')
    for w in warnings:
        print('WARNING:', w, file=sys.stderr)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
