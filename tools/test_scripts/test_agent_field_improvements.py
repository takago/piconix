#!/usr/bin/env python3
"""Static regression checks for LLM field-test changes."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
cc=(root/'userland/apps/cc.c').read_text()
assert 'static void incdec(' in cc and 'static int step_token(' in cc
assert 'int post=step_token()' in cc and 'int step=step_token()' in cc
assert 'if(used>ASM_FILE_MAX)' in cc
pins=(root/'kernel/pins.h').read_text()
assert 'pin_reserved' in pins and 'pin==25' in pins
assert 'return !pin_reserved(pin)' in pins
gpio=(root/'kernel/gpio.c').read_text()
assert '#if !PICONIX_WIFI' in gpio
assert 'GPIO_STATUS_RESERVED' in (root/'kernel/kernel.c').read_text()
assert all((root/'manual'/x).exists() for x in ('shell-syntax','compatibility','process-resources','gpio-board'))
print('PASS: compiler tokens, Pico W GPIO guards and manual pages')
