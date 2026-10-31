#!/usr/bin/env python3
"""Guard the bounded argv ABI and /bin virtual registry."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
k=(root/'kernel/kernel.c').read_text()
sh=(root/'userland/shell.c').read_text()
api=(root/'userland/unistd.h').read_text()
assert 'int spawn(int program, int argc, const char *const argv[]);' in api
assert 'argc<1 || argc>8' in k
assert 'unsigned total=0;' in k and 'len<128-total' in k
assert 'argv[argc]=NULL;' in k and 'char *strings=(char *)(top-192);' in k
assert 'case VDIR_BIN: case VDIR_USERLAND:' in k and 'getdents' in (root/'userland/ls.c').read_text()
assert 'program_id(name)' in sh and 'run_external(s,cwd,requested_bg' in sh
assert 'args[argc++]="31"' in sh
assert 'int count=argc-1;' in (root/'userland/hexdump.c').read_text()
print('PASS: bounded argv ABI, /bin registry and application parser')
