#!/usr/bin/env python3
"""Check that ls retrieves IDs via getdents rather than an independent table."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
h = (root / 'include/piconix_dirent.h').read_text()
k = (root / 'kernel/kernel.c').read_text()
ls = (root / 'userland/apps/ls.c').read_text()
assert 'uint16_t device_id' in h
assert 'entry.device_id=vdir_entry_device_id(dir,entry.name,entry.is_dir)' in k
assert 'vfs_fixed_path_id(path)' in k
assert 'DEV_GPIO_VALUE_BASE+(int)pin' in k
assert 'e.device_id' in ls
assert '65535u' in ls
assert 'TYPE    ID   SIZE NAME' in ls
assert 'dir==VDIR_BIN || dir==VDIR_USERLAND' in k
assert 'dir==VDIR_TMP || dir==VDIR_ETC' in k
assert 'return (uint16_t)i;' in k
print('PASS: kernel-provided device IDs in ls -l')
