#!/usr/bin/env python3
"""The /dev/tty path is listed and resolves at open to the calling task's TTY."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
h = (root / 'include/device_names.h').read_text()
k = (root / 'kernel/kernel.c').read_text()
assert 'X("/dev/tty", DEV_CTTY)' in h
assert 'PICONIX_FIXED_DEVICES(DEVICE_ENTRY)' in k
assert 'vfs_device_path(i)' in k  # /dev listing comes from the same table
assert 'if(dev==DEV_CTTY) dev=tty_device(tasks[current].tty);' in k
for tty, device in [(0, 'DEV_CONSOLE'), (1, 'DEV_UART1'),
                    (2, 'DEV_WIFI_TTY'), (3, 'DEV_PTY_SLAVE'),
                    (4, 'DEV_CDC_VCONSOLE')]:
    token = 'if(tty==%s) return %s;' % (tty, device)
    if tty == 3:
        token = 'if(tty==PTY_TTY) return DEV_PTY_SLAVE;'
    if tty == 4:
        token = 'if(tty==CDC_VCONSOLE_TTY) return DEV_CDC_VCONSOLE;'
    assert token in k, token
assert 'if(dev<0 ||' in k  # Unknown controlling TTY is rejected.
assert 'if(dev==DEV_CDC_ACTIVE)' in k  # /dev/console remains distinct.
print('PASS: /dev/tty inventory and caller-controlled open resolution')
