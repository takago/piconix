#!/usr/bin/env python3
"""Derive PicoNIX RP2040 XIP linker layout from Pico SDK 2.1.1 default script.

Only writable kernel allocations use the first 64 KiB of main SRAM.
Task slots and the Pico W networking reservation remain outside this region.
"""
import pathlib
import re
import sys


def transform(source):
    ram = re.compile(r'(?m)^(?P<pre>\s*RAM\s*\(\s*rwx\s*\)\s*:\s*ORIGIN\s*=\s*0x20000000\s*,\s*LENGTH\s*=\s*)(?:256\s*[kK]|0x40000|262144)(?P<post>\s*)$')
    if len(list(ram.finditer(source))) != 1:
        raise ValueError('expected one 256-KiB RAM declaration in SDK XIP script')
    if not re.search(r'(?m)^\s*(?:INCLUDE\s+["\']?pico_flash_region\.ld|FLASH\s*\()', source):
        raise ValueError('SDK script lacks FLASH region')
    source = ram.sub(lambda m: m['pre'] + '64k' + m['post'], source, count=1)
    # Fail closed if the SDK changes the standard XIP layout.
    text = re.search(r'(?s)\n\s*\.text\s*:\s*\{.*?\}\s*>\s*FLASH', source)
    rodata = re.search(r'(?s)\n\s*\.rodata\s*:\s*\{.*?\}\s*>\s*FLASH', source)
    if not text or not rodata:
        raise ValueError('SDK default linker script must place .text and .rodata in FLASH')
    if re.search(r'(?s)\n\s*\.data\s*:\s*\{.*?\}\s*>\s*RAM', source) is None:
        raise ValueError('SDK default linker script must place .data in RAM')
    if '(.piconix_app_image' in source:
        raise ValueError('application image section already present')
    # Place the embedded images after the SDK flash code/rodata but before RAM
    # initializers. Preserve flash vectors and the boot2 offsets exactly.
    match = re.search(r'(?m)^[ \t]*\.data\s*:', source)
    if not match:
        raise ValueError('missing .data section')
    app = '''    /* PicoNIX application binaries and symbol tables stay in XIP FLASH. */
    .piconix_app_images :
    {
        . = ALIGN(4);
        __piconix_app_images_start = .;
        KEEP(*(.piconix_app_image*))
        KEEP(*(.rodata.piconix_symbols))
        KEEP(*(.rodata.piconix_kernel_symbols))
        . = ALIGN(4);
        __piconix_app_images_end = .;
    } > FLASH

'''
    # Note: this section must precede .data's FLASH load image.
    return '/* PicoNIX XIP: flash code/const, 64-KiB kernel RAM. */\n' + source[:match.start()] + app + source[match.start():]


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: limit_sdk_xip.py sdk_memmap_default.ld output.ld')
    try:
        src = pathlib.Path(sys.argv[1]).read_text()
        pathlib.Path(sys.argv[2]).write_text(transform(src))
    except (OSError, ValueError) as exc:
        sys.exit(f'ERROR: {exc}')

if __name__ == '__main__':
    main()
