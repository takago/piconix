#!/usr/bin/env python3
"""Host-side regression tests for standalone Cortex-M0+ RAM application images.

Requires clang, ld.lld, llvm-objcopy, readelf; does not require Pico SDK.
"""
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
CLANG = shutil.which('clang')
LLD = shutil.which('ld.lld')
OBJCOPY = shutil.which('llvm-objcopy')
READELF = shutil.which('readelf')


def run(*args):
    subprocess.run([str(x) for x in args], check=True, cwd=ROOT, env={**os.environ, 'LC_ALL': 'C', 'LANG': 'C'})


def test_app(tmp, name, base):
    obj = tmp / f'{name}.o'
    elf = tmp / f'{name}.elf'
    binary = tmp / f'{name}.bin'
    run(CLANG, '--target=arm-none-eabi', '-mcpu=cortex-m0plus', '-mthumb',
        '-Os', '-ffreestanding', '-fno-builtin', '-ffunction-sections',
        '-fdata-sections', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables',
        '-c', ROOT / 'ram_apps' / f'{name}.c', '-o', obj)
    linker_script = tmp / f'{name}.ld'
    linker_script.write_text((ROOT / 'ram_apps' / 'slot.ld').read_text().replace('@APP_ORIGIN@', f'{base:#x}'))
    run(LLD, '-flavor', 'gnu', '-m', 'armelf', '-T',
        linker_script, '--gc-sections', obj, '-o', elf)
    run(OBJCOPY, '-O', 'binary', elf, binary)
    run(sys.executable, ROOT / 'tools' / 'check_slot_image.py',
        elf, binary, hex(base), '--readelf', READELF)
    # These are self-contained task images: no unresolved external symbols,
    # dynamic relocations, or runtime dependencies on the host firmware.
    sections = subprocess.check_output([READELF, '-SW', str(elf)], text=True, env={**os.environ, 'LC_ALL': 'C', 'LANG': 'C'})
    assert '.dynamic' not in sections and '.got' not in sections
    symbols = subprocess.check_output([READELF, '-sW', str(elf)], text=True, env={**os.environ, 'LC_ALL': 'C', 'LANG': 'C'})
    undefined = [line for line in symbols.splitlines() if ' UND ' in line and line.split()[-1] != 'UND']
    assert not undefined, f'{name}: undefined symbols: {undefined}'
    reloc = subprocess.check_output([READELF, '-rW', str(elf)], text=True, env={**os.environ, 'LC_ALL': 'C', 'LANG': 'C'})
    assert 'There are no relocations in this file.' in reloc, reloc
    print(f'PASS: {name} image ({binary.stat().st_size} bytes)')


def main():
    missing = [name for name, path in [('clang', CLANG), ('ld.lld', LLD),
               ('llvm-objcopy', OBJCOPY), ('readelf', READELF)] if not path]
    if missing:
        sys.exit('Missing tools: ' + ', '.join(missing))
    with tempfile.TemporaryDirectory(prefix='piconix-ram-test-') as directory:
        tmp = pathlib.Path(directory)
        test_app(tmp, 'shell', 0x20010000)
        test_app(tmp, 'blink', 0x20020000)
    print('PASS: both fixed-RAM images have no unresolved references or relocations')


if __name__ == '__main__':
    main()
