#!/usr/bin/env python3
"""Guard top's RAM/PIC image against absolute-address pointer tables."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
clang = shutil.which('clang')
readelf = shutil.which('readelf')
if not clang or not readelf:
    print('SKIP: requires clang and readelf')
else:
    with tempfile.TemporaryDirectory() as directory:
        obj = Path(directory) / 'top.o'
        subprocess.run([
            clang, '--target=arm-none-eabi', '-mcpu=cortex-m0plus', '-mthumb',
            '-Oz', '-fPIC', '-ffreestanding', '-fno-builtin',
            '-ffunction-sections', '-fdata-sections', '-fno-jump-tables',
            '-fno-unwind-tables', '-fno-asynchronous-unwind-tables',
            '-I', str(root / 'include'), '-I', str(root / 'userland/lib'),
            '-c', str(root / 'userland/apps/top.c'), '-o', str(obj),
        ], check=True)
        relocations = subprocess.check_output([readelf, '-rW', str(obj)], text=True)
        assert 'R_ARM_ABS32' not in relocations, 'top has absolute address relocations'
    print('PASS: top object has no R_ARM_ABS32 relocations')
