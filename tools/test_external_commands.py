#!/usr/bin/env python3
"""Source-level checks for external commands and working directory syscalls."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'userland/shell.c').read_text(); k=(r/'kernel/kernel.c').read_text()
for old in ('else if(eq(s,"pwd"))','else if(eq(s,"pid"))','else if(eq(s,"tty"))','else if(prefix(s,"kill "))','else if(eq(s,"ls")','else if(prefix(s,"rm "))','else if(prefix(s,"echo "))'):
    assert old not in s, old
for call in ('SYS_CHDIR','SYS_GETCWD','absolute_path','strcpy(t->cwd','vfs_open(path,b)','tmp_unlink(path)'):
    assert call in k, call
for name in ('pwd','ls','rm','echo','kill','tty'):
    assert (r/'userland'/f'{name}.c').exists()
print('PASS: external commands and cwd syscall integration')
