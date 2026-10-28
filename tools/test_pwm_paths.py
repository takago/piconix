#!/usr/bin/env python3
"""PWM slice 1 namespace and client consistency checks."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
k=(root/'src/kernel.c').read_text()
ls=(root/'ram_apps/ls.c').read_text()
ctl=(root/'ram_apps/pwmctl.c').read_text()
for name in ('freq','a','b'):
    path=f'/dev/pwm/1/{name}'
    assert path in k and path in ctl
assert '/dev/pwm/1' in k and '/dev/pwm/1' in ls
assert 'put("1/\\r\\n")' in ls or 'put("1/\r\n")' in ls
for old in ('/dev/pwm/freq','/dev/pwm/18','/dev/pwm/19'):
    for source in root.rglob('*'):
        if source.is_file() and source.suffix in ('.c','.h','.md'):
            assert old not in source.read_text(), (old,source)
assert 'eq(argv[1],"a")' in ctl and 'eq(argv[1],"b")' in ctl
assert '"/dev/pwm/1/a"' in (root/'ram_apps/kitesys.c').read_text()
print('PASS: PWM slice 1 device paths and clients')
