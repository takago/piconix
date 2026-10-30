#!/usr/bin/env python3
"""Source-level regression checks for hierarchical Flash-backed symbol catalogs."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
k=(root/'kernel/kernel.c').read_text()
l=(root/'userland/ls.c').read_text()
m=(root/'userland/mem.c').read_text()
assert '/proc/syms' not in k and 'DEV_SYMS = ' not in k
assert '{"/proc/sym/kernel",DEV_SYMS_KERNEL}' in k
assert '!strcmp(path,"/proc/sym")' in k and '!strcmp(path,"/proc/sym/userland")' in k
assert '"/proc/sym/userland/"' in k and 'DEV_SYMS_USER_BASE+id' in k
assert 'piconix_symbols_start' in k and 'line[name_len]' in k
assert 'piconix_kernel_syms_start+4u' in k
assert '"/proc/sym/userland/%s"' in k
assert 'getdents(fd,&e,sizeof e)' in l
assert 'case VDIR_SYM:' in k and 'EMIT("userland",1)' in k
assert '"/proc/sym/kernel"' in m and '"/proc/sym/userland/%s"' in m
assert '/proc/syms' not in m and 'list_symbol_sources()' in m
print('PASS: hierarchical symbol paths, FD naming, mem and removed legacy catalog')
