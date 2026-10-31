#!/usr/bin/env python3
"""Guard against sparse registry IDs and accidental ordinal lookup."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
k=(r/'kernel/kernel.c').read_text()
s=(r/'userland/shell.c').read_text()
t=(r/'userland/trace.c').read_text()
w=(r/'userland/watch.c').read_text()
u=(r/'userland/syscalls.c').read_text()
assert 'SYS_PIPE, SYS_DUP, SYS_PROGRAM_ID' in k
assert 'case SYS_PROGRAM_ID:' in k
assert 'for(int id=1;id<PROGRAM_CAPACITY;id++)' in k
assert '!strcmp(lookup,programs[id].name)' in k
assert 'in_task_range(name+n,1)' in k
assert 'call(38,(uintptr_t)name,0,0)' in u
assert 'static int program_id(' not in s+w+t
assert 'program_id(name)' in s+w+t
assert 'int actual_id=program_id(ent.name)' in t
assert 'eq(name,"python")' in s and 'eq(name,"hexdump")' in s and 'eq(name,"cat")' in s
print('PASS: kernel registry lookup, callers and sparse ID mapping')
