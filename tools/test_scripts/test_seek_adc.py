#!/usr/bin/env python3
"""Source-level guards for ADC repeat reads and lseek ABI; hardware tests still required."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
k = (root / "kernel/kernel.c").read_text()
h = (root / "userland/unistd.h").read_text()
p = (root / "userland/python.c").read_text()
t = (root / "userland/fs/tmp_common/test.py").read_text()
assert 'X(SYS_LSEEK,         21' in (root / 'include/piconix_syscall.h').read_text()
assert 'pos==(size_t)len ? 0 : pos' in k
assert 'if(c==0) base=0;' in k and 'else if(c==1)' in k and 'else if(c==2)' in k
assert 'tmp_files[dev-DEV_TMP_FIRST].size' in k
assert 'memset(tmp_payload(id)+tmp_files[id].size,0,pos-tmp_files[id].size)' in k
assert '#define SEEK_SET 0' in h and '#define SEEK_CUR 1' in h and '#define SEEK_END 2' in h
assert 'if(same(name,"lseek")&&eat' in p
assert t.count('open("/dev/adc/temp"') == 1
assert 'for i in range(5)' in t and 'temp = read(sensor, 32)' in t
print('PASS: ADC repeat-read and lseek source checks')
