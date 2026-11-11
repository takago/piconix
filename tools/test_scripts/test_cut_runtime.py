#!/usr/bin/env python3
"""Compile and execute the real cut.c on the host using libc I/O."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
cc=shutil.which('clang') or shutil.which('gcc')
if cc is None:
    raise SystemExit('SKIP: host C compiler unavailable')
with tempfile.TemporaryDirectory() as d:
    exe=Path(d)/'cut'
    stub=Path(d)/'stub.c'
    stub.write_text('int msleep(unsigned ms) {(void)ms;return 0;}\n')
    subprocess.run([cc,'-DPICONIX_HOST_TEST','-I'+str(root/'include'),
                    '-I'+str(root/'userland/lib'),str(root/'userland/apps/cut.c'),
                    str(stub),'-o',str(exe)],check=True,capture_output=True)
    def run(args,data):
        return subprocess.run([str(exe),*args],input=data.encode(),capture_output=True)
    assert run(['-d,','-f1,4'],'time,busy,idle,network\n10,2,98,4\n').stdout == b'time,network\n10,4\n'
    assert run(['-d',',','-f','2-3'],'a,b,c,d\n1,2,3,4').stdout == b'b,c\n2,3'
    assert run(['-f1'],'one\ttwo\n').stdout==b'one\n'
    assert run(['-d,','-f3,1'],'a,b,c\n').stdout==b'a,c\n'
    assert run(['-d,','-f3'],'plain\n').stdout==b'plain\n'
    assert run(['-d,','-f2'],'a,,c\n').stdout==b'\n'
    assert run(['-d,','-f1'],'a,b\n\n').stdout==b'a\n\n'
    assert run(['-d,','-f65'],'a,b\n').returncode==2
    assert run(['-d,','-f3-2'],'a,b\n').returncode==2
    assert run(['-d,','-f1'],'x'*513+'\n').returncode==1
    data='a,b,c\n'
    path=Path(d)/'data.csv';path.write_text(data)
    assert run(['-d,','-f2',str(path)],'').stdout==b'b\n'
print('PASS: cut host runtime, ranges, EOF, invalid args, long lines, files')
