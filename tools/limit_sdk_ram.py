#!/usr/bin/env python3
"""Derive RP2040 SDK copy_to_ram linker script with a 96-KiB kernel.

Embedded user images stay in XIP flash, rather than the RAM .data output
section.  Reject unknown SDK script layouts instead of silently mislinking.
"""
import pathlib
import re
import sys


def transform(source):
    pat = re.compile(r"(?m)^(?P<pre>[ \t]*RAM[ \t]*\([ \t]*rwx[ \t]*\)[ \t]*:[ \t]*ORIGIN[ \t]*=[ \t]*0x20000000[ \t]*,[ \t]*LENGTH[ \t]*=[ \t]*)(?P<size>256[ \t]*[kK]|0x40000|262144)(?P<post>[ \t]*)$")
    if len(list(pat.finditer(source))) != 1:
        raise ValueError('expected exactly one 256-KiB RP2040 RAM declaration')
    # Pico SDK 2.1.1 declares FLASH via INCLUDE "pico_flash_region.ld".
    # Accept either that SDK include or an explicit FLASH memory declaration.
    if not (re.search(r'(?m)^\s*INCLUDE\s+["\']?pico_flash_region\.ld', source)
            or re.search(r'(?m)^\s*FLASH\s*\(', source)):
        raise ValueError('missing FLASH region or pico_flash_region.ld include')
    source = pat.sub(lambda m: m['pre'] + '96k' + m['post'], source, count=1)
    # Insert before RAM-resident .text. The FLASH output section advances the
    # FLASH load region so subsequent AT> FLASH sections do not overlap it.
    text_pat = re.compile(r'(?m)^(?P<indent>[ \t]*)\.text[ \t]*:')
    matches = list(text_pat.finditer(source))
    if len(matches) != 1:
        raise ValueError(f'expected exactly one .text output section; found {len(matches)}')
    if 'AT> FLASH' not in source and 'AT > FLASH' not in source:
        raise ValueError('expected SDK RAM copy sections loaded from FLASH')
    m = matches[0]
    section = (m['indent'] + '/* User application binaries are stored in XIP flash only. */\n'
               + m['indent'] + '.piconix_app_images :\n'
               + m['indent'] + '{\n'
               + m['indent'] + '    . = ALIGN(4);\n'
               + m['indent'] + '    __piconix_app_images_start = .;\n'
               + m['indent'] + '    KEEP(*(.piconix_app_image*))\n'
               + m['indent'] + '    . = ALIGN(4);\n'
               + m['indent'] + '    __piconix_app_images_end = .;\n'
               + m['indent'] + '} > FLASH\n\n')
    return ('/* PicoNIX: 96-KiB RAM kernel; user binaries resident in FLASH. */\n'
            + source[:m.start()] + section + source[m.start():])


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: limit_sdk_ram.py sdk_memmap_copy_to_ram.ld output.ld')
    try:
        source = pathlib.Path(sys.argv[1]).read_text()
        pathlib.Path(sys.argv[2]).write_text(transform(source))
    except (OSError, ValueError) as exc:
        sys.exit(f'ERROR: {exc}')


if __name__ == '__main__':
    main()
