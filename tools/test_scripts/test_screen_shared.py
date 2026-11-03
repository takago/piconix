#!/usr/bin/env python3
"""Structural checks and host syntax checks for shared ANSI terminal helpers."""
from pathlib import Path
import subprocess
import shutil
root=Path(__file__).resolve().parents[2]
u=root/'userland/apps'
lib=root/'userland/lib'
screen=(lib/'screen.h').read_text()
for symbol in ('screen_measure(', 'screen_enter(', 'screen_leave(', 'screen_move('):
    assert symbol in screen
assert 'O_NONBLOCK' in screen and 'F_SETFL,flags)' in screen
for name in ('edit','vi'):
    src=(u/'editor_common.h').read_text()
    assert '#include "../lib/screen.h"' in src
    assert 'screen_measure(&size)' in src
for name in ('mb','mem'):
    assert '#include "../lib/screen.h"' in (u/f'{name}.c').read_text()
assert 'screen_measure(&terminal)' in (u/'mb.c').read_text()
assert 'screen_move(top+row,left)' in (u/'mb.c').read_text()
cc=shutil.which('gcc') or shutil.which('clang')
assert cc
for name in ('edit','vi','mem','mb'):
    subprocess.run([cc,'-std=c11','-Wall','-Wextra','-fsyntax-only','-I',str(lib),str(u/f'{name}.c')], check=True)
print('PASS: shared screen helpers and four application consumers')
