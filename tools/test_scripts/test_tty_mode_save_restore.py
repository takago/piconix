#!/usr/bin/env python3
"""Ensure TTY foreground save/restore preserves every mode flag and the input queue."""
from pathlib import Path
import re
source = (Path(__file__).resolve().parents[2] / 'kernel/kernel.c').read_text()

def function(name):
    match = re.search(r'NO_TRACE static void '+name+r'\(int dev\) \{(.*?)\n\}', source, re.S)
    assert match, name
    return match.group(1)

save, restore = function('tty_mode_save'), function('tty_mode_restore')
for field, saved in [('tty_canonical','tty_saved_canon'),('tty_echo','tty_saved_echo'),
                     ('tty_onlcr','tty_saved_onlcr'),('tty_interrupt','tty_saved_interrupt'),
                     ('tty_mode_owner','tty_saved_owner')]:
    assert f'{saved}[dev]={field}[dev];' in save
    assert f'{field}[dev]={saved}[dev];' in restore
assert 'tty_last_cr[dev]=0;' in restore
assert 'tty_buf' not in save and 'tty_buf' not in restore
assert 'tty_mode_save(tty);' in source
assert 'tty_mode_restore(dev);' in source
print('PASS: foreground TTY save/restore preserves modes and queued input')
