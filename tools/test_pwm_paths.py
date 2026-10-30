#!/usr/bin/env python3
"""PWM slice 1 namespace and client consistency checks."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
k=(root/'kernel/kernel.c').read_text()
v=k
h=k
ls=(root/'userland/ls.c').read_text()
ctl=(root/'userland/pwmctl.c').read_text()
for name in ('freq','a','b'):
    path=f'/dev/pwm/1/{name}'
    assert path in v and path in ctl
assert '/dev/pwm/1' in k and 'getdents(fd,&e,sizeof e)' in ls
assert 'case VDIR_PWM:' in k and 'EMIT("1",1)' in k
for old in ('/dev/pwm/freq','/dev/pwm/18','/dev/pwm/19'):
    for source in root.rglob('*'):
        if source.is_file() and source.suffix in ('.c','.h','.md'):
            assert old not in source.read_text(), (old,source)
assert 'eq(argv[1],"a")' in ctl and 'eq(argv[1],"b")' in ctl
assert '"/dev/pwm/1/a"' in (root/'userland/kitesys.c').read_text()
print('PASS: PWM slice 1 device paths and clients')
