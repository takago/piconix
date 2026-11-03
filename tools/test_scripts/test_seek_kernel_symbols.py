#!/usr/bin/env python3
"""Source-level regression check for seekable /proc/sym/kernel."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
src = (ROOT / 'kernel/kernel.c').read_text()
start = src.index('case SYS_LSEEK:')
end = src.index('\n        }\n    }\n    if(num==SYS_OPEN', start)
branch = src[start:end]
assert 'dev==DEV_SYMS_KERNEL' in branch
assert 'piconix_kernel_syms_end-piconix_kernel_syms_start' in branch
assert 'length>reserved-4u' in branch
assert '(dev==DEV_SYMS_KERNEL)?end:TMP_BYTES' in branch
for x in ('if(c==0) base=0', 'else if(c==1)', 'else if(c==2) base=end', 'pos<0 || pos>limit'):
    assert x in branch, x
# Check expected range/whence semantics, including the EOF boundary.
def seek(current, end, offset, whence):
    base = (0, current, end)[whence]
    pos = base + offset
    return pos if 0 <= pos <= end else -1
assert seek(4, 123, 0, 2) == 123
assert seek(4, 123, -5, 2) == 118
assert seek(4, 123, 5, 0) == 5
assert seek(4, 123, -5, 1) == -1
assert seek(4, 123, 124, 0) == -1
print('PASS: kernel symbol seek source and range semantics')
