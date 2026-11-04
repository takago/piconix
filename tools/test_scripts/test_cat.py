#!/usr/bin/env python3
"""Exercise cat's byte-stream copying and -n line numbering on a host PC."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
cc = shutil.which('cc')
if not cc:
    raise SystemExit('Missing native C compiler')

with tempfile.TemporaryDirectory(prefix='piconix-cat-') as tmp:
    dest = Path(tmp)
    shim = dest / 'shim.c'
    shim.write_text('int msleep(unsigned n) { (void)n; return 0; }\n')
    binary = dest / 'cat'
    subprocess.run([cc, '-std=c11', '-Wall', '-Wextra', '-Werror', '-fno-builtin', '-DPICONIX_HOST_TEST',
                    '-I', str(root / 'userland/lib'), '-I', str(root / 'include'),
                    str(root / 'userland/apps/cat.c'), str(root / 'userland/lib/libc.c'),
                    str(shim), '-o', str(binary)],
                   check=True)

    def run(*args, data=b''):
        return subprocess.run([str(binary), *args], input=data, capture_output=True)

    def numbered(data):
        result = bytearray()
        line_number = 1
        for line in data.split(b'\n')[:-1]:
            result += f'{line_number:6d}\t'.encode() + line + b'\n'
            line_number += 1
        last = data.split(b'\n')[-1]
        if last:
            result += f'{line_number:6d}\t'.encode() + last
        return bytes(result)

    # Normal cat remains a byte-for-byte copy, including embedded NULs.
    for content in (b'', b'hello\n', b'no newline', b'\n\n', b'abc\x00def\n',
                    b'x'*99 + b'\n' + b'\n' + b'z'*73,
                    b'one\r\ntwo\rthree\vfour\f\n', b'\x80\xff\x00\n'):
        assert run(data=content).stdout == content
        test = run('-n', data=content)
        assert test.returncode == 0, test.stderr
        assert test.stdout == numbered(content), repr(test.stdout)

    # Line numbering must not restart when a 32-byte read buffer fills.
    content = b'\n'.join((f'line {i}: '.encode() + b'a'*(i % 49)) for i in range(100)) + b'\n'
    test_file = dest / 'data.txt'
    test_file.write_bytes(content)
    for args, expected in (((str(test_file),), content),
                           (('-n', str(test_file)), numbered(content))):
        result = run(*args)
        assert result.returncode == 0 and result.stdout == expected, args

    assert run('-n', data=b'last').stdout == b'     1\tlast'
    assert run('-n', data=b'line\n').stdout == b'     1\tline\n'
    assert run('-n', str(test_file), 'extra').returncode == 1
    assert b'usage: cat [-n] [PATH]' in run('-n', str(test_file), 'extra').stdout
    assert b'cannot open' in run(str(dest / 'missing')).stdout
    assert b'console reserved for shell' in run('/dev/console').stdout

print('PASS: cat unnumbered/numbered stdin and files, blank/long/binary lines, errors')
