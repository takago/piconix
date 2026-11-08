#!/usr/bin/env python3
"""Host test for cc output limits; generated assembly is not run on ARM."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'userland/apps/cc.c').read_text()
for header in ('unistd', 'fcntl', 'stdio', 'string'):
    source = source.replace(f'#include "{header}.h"', f'#include <{header}.h>')

with tempfile.TemporaryDirectory() as temp:
    base = Path(temp)
    (base / 'cc_host.c').write_text(source)
    subprocess.run(['cc', '-std=c11', '-O0', str(base / 'cc_host.c'), '-o', str(base / 'cc_host')], check=True)

    def compile_program(count):
        path = base / 'sample.c'
        path.write_text('int main(){int a=0;' + 'a=a+1;' * count + 'return a;}')
        return subprocess.run([str(base / 'cc_host'), str(path)], capture_output=True, text=True)

    good = compile_program(18)
    assert good.returncode == 0, good.stdout + good.stderr
    asm = base / 'sample.s'
    assert 0 < asm.stat().st_size <= 2048
    original = asm.read_bytes()

    too_big = compile_program(24)
    assert too_big.returncode != 0
    assert 'RAM file limit (2048 bytes)' in too_big.stdout
    assert asm.read_bytes() == original, 'failed generation overwrote previous output'

    (base / 'sample.s').unlink()
    again = compile_program(24)
    assert again.returncode != 0
    assert not (base / 'sample.s').exists(), 'overflow left partial .s'

    internal = compile_program(65)
    assert internal.returncode != 0
    assert 'internal buffer (4096 bytes)' in internal.stdout
    assert not (base / 'sample.s').exists()

print('PASS: cc diagnostics distinguish RAM-file/internal-output limits and preserve files')
