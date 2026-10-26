#!/usr/bin/env python3
"""Derive an SDK-compatible copy_to_ram linker script limited to 64 KiB.

Fail closed if the SDK MEMORY layout is not a recognized RP2040 shape.
Do not silently mutate scripts for other targets/SDK revisions.
"""
import pathlib
import re
import sys


def transform(source):
    # Pico SDK RAM declaration is typically RAM(rwx) : ORIGIN = 0x20000000, LENGTH = 256k
    pat = re.compile(r"(?m)^(?P<pre>[ \t]*RAM[ \t]*\([ \t]*rwx[ \t]*\)[ \t]*:[ \t]*ORIGIN[ \t]*=[ \t]*0x20000000[ \t]*,[ \t]*LENGTH[ \t]*=[ \t]*)(?P<size>256[ \t]*[kK]|0x40000|262144)(?P<post>[ \t]*)$")
    matches = list(pat.finditer(source))
    if len(matches) != 1:
        raise ValueError(f"expected exactly one 256-KiB main RAM region; found {len(matches)}")
    result = pat.sub(lambda m: m.group('pre') + '64k' + m.group('post'), source, count=1)
    if '0x20000000' not in result or 'FLASH' not in result:
        raise ValueError('unexpected SDK linker script')
    return '/* PicoNIX Stage 17: derived from Pico SDK copy_to_ram; kernel limited to 64 KiB. */\n' + result


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: limit_sdk_ram.py sdk_memmap_copy_to_ram.ld output.ld')
    try:
        source = pathlib.Path(sys.argv[1]).read_text()
        result = transform(source)
        pathlib.Path(sys.argv[2]).write_text(result)
    except (OSError, ValueError) as exc:
        sys.exit(f'ERROR: {exc}')


if __name__ == '__main__':
    main()
