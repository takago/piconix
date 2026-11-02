#!/usr/bin/env python3
"""Exercise both post-link Wi-Fi and plain Pico bank checks using mock nm."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='piconix_layout_') as temp:
    p = Path(temp)
    nm = p/'nm'
    def run(stack, wifi, ok):
        nm.write_text(f'''#!/bin/sh
printf '%s\\n' '2000e000 A task_ram' '20001000 B tasks' '{stack:08x} A idle_stack' '20032000 A shared_ram' '20032800 A ramfile_data' '20038000 A trace_ring'
''')
        nm.chmod(0o755)
        command = ['python3', str(root/'tools/verify_layout.py'), str(p/'dummy.elf'), '--nm', str(nm)]
        if wifi: command.append('--wifi')
        result = subprocess.run(command,capture_output=True,text=True)
        assert (result.returncode == 0) == ok, (command, result.stdout,result.stderr)
    run(0x2003F000, True, True)
    run(0x20002000, False, True)
    run(0x20002000, True, False)
    run(0x2003F000, False, False)
print('PASS: Pico/Pico W ELF idle-stack placement and heap separation guarded')
