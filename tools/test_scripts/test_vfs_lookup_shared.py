#!/usr/bin/env python3
"""Keep RAM-file directory IDs and proc-task open lookup on existing helpers."""
from pathlib import Path
import re

root=Path(__file__).resolve().parents[2]
s=(root/'kernel/kernel.c').read_text()
start=s.index('NO_TRACE static uint16_t vdir_entry_device_id(')
end=s.index('/* ディレクトリFDの現在位置',start)
entry=s[start:end]
assert 'tmp_lookup(path+1)' in entry
assert 'tmp_files[i].used && !strncmp' not in entry
assert 'return index>=0 ? (uint16_t)index : 0xffff;' in entry
start=s.index('static int vfs_open(')
end=s.index('static int device_rw_rand(',start)
opened=s[start:end]
assert 'slot_from_pid((int)pid)' in opened
assert 'slot<OS_MAX_TASKS' in opened
assert 'tasks[i].pid==(int)pid' not in opened
# Entry metadata must still reserve 0xffff for non-file or missing objects.
assert 'return id>=0 && id<65535 ? (uint16_t)id : 0xffff;' in entry
assert 'static int tmp_lookup(const char *name)' in s
assert 'NO_TRACE static int slot_from_pid(int pid)' in s
print('PASS: VFS reuses RAM-file lookup and live PID lookup')
