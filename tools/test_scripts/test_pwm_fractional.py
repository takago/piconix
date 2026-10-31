#!/usr/bin/env python3
"""Check the 0.1%-resolution PWM interface and formatting."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
k = (root / 'kernel/kernel.c').read_text()
pwm = (root / 'kernel/pwm.c').read_text()
ctl = (root / 'userland/pwmctl.c').read_text()
assert "tenths=x*10u" in pwm
assert "tenths>1000" in pwm
assert '"%u.%u\\n"' in k or '"%u.%u\n"' in k
assert 'pwm_duty[0]==1000' in pwm and 'pwm_duty[1]==1000' in pwm
assert 'period*pwm_duty[0]/1000u' in pwm
assert 'valid_value(' in ctl and 'set_value(path,argv[2])' in ctl
print('PASS: fractional PWM duty cycle interface')
