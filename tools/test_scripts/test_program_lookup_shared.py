#!/usr/bin/env python3
"""Program IDs and VFS/symbol lookup must use one name-to-ID implementation."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
helper=k.split('NO_TRACE static int registered_program_id(',1)[1].split('\n}',1)[0]
assert 'for (int id=1;id<PROGRAM_CAPACITY;id++)' in helper
assert 'programs[id].entry && programs[id].name' in helper
assert '!strcmp(name,programs[id].name)' in helper
assert k.count('registered_program_id(name)') == 2
assert k.count('registered_program_id(lookup)') == 1
assert 'if (id>0) return (uint16_t)id;' in k
assert 'if (id>0) dev=DEV_SYMS_USER_BASE+id;' in k
assert 'result=registered_program_id(lookup);' in k
print('PASS: program name lookup shared across VFS and syscall')
