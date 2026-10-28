#!/usr/bin/env python3
"""Regression checks: pins uses existing GPIO/PWM devices, not /dev/pins."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
k=(r/'src/kernel.c').read_text()
a=(r/'ram_apps/pins.c').read_text()
h=(r/'include/gpio_status.h').read_text()
assert not (r/'include/pins_snapshot.h').exists()
assert 'DEV_PINS' not in k and '/dev/pins' not in k
assert 'OS_GPIO_STATUS_BASE' in k and 'gpio_get_function(pin)' in k
assert 'fb_active()' in k and 'gpio_get_dir(pin)' in k
assert 'OS_GPIO_STATUS_BASE 0x400' in h
assert 'open("/dev/gpio/0",O_RDONLY)' in a
assert 'OS_GPIO_STATUS_BASE+pin' in a
assert 'open("/dev/pwm/1/freq",O_RDONLY)' in a
assert 'pwm %u%% %uHz' in a
assert 'pinout' not in (r/'src/main.c').read_text()
print('PASS: pins uses existing GPIO and PWM devices')
