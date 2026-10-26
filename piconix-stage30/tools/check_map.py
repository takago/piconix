#!/usr/bin/env python3
"""Check ELF SRAM usage and reserved task area via arm-none-eabi-size.
This is a Stage 2 diagnostic, not proof of an all-RAM image.
"""
import argparse, subprocess, re, sys
p=argparse.ArgumentParser()
p.add_argument('elf')
a=p.parse_args()
try:
    out=subprocess.check_output(['arm-none-eabi-size','-A',a.elf],text=True)
except (OSError,subprocess.CalledProcessError) as e:
    sys.exit(f'Unable to inspect ELF: {e}')
print(out)
print('NOTE: Stage 2 uses XIP for .text; task_ram is reserved in .bss.')
print('NOTE: This report is not a RAM-only linker validation.')
