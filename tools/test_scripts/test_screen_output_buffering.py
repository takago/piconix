#!/usr/bin/env python3
"""Ensure buffered full-screen renderers flush before yielding or input."""
from pathlib import Path
import subprocess
import shutil
root=Path(__file__).resolve().parents[2]
apps=root/'userland/apps'
buffered=('top','fd','mem','mb','reversi','sudoku','samegame','maze','tictactoe')
for app in buffered:
    source=(apps/f'{app}.c').read_text()
    assert 'setvbuf(stdout,0,_IOFBF,0)' in source, app
    assert 'fflush(stdout)' in source, app
for app in ('vi','edit','pong'):
    source=(apps/f'{app}.c').read_text()
    assert 'setvbuf(stdout,0,_IOFBF,0)' not in source, app
screen=(root/'userland/lib/screen.h').read_text()
assert 'setvbuf(stdout,0,_IONBF,0)' in screen
assert 'setvbuf(stdout,0,_IOLBF,0)' in screen
assert 'fflush(stdout)' in (apps/'fd.c').read_text().split('static void draw_dir(void)')[1].split('static int safe_view')[0]
assert 'fflush(stdout)' in (apps/'fd.c').read_text().split('static void paint_view(')[1].split('static unsigned view_hash')[0]
assert 'fflush(stdout)' in (apps/'mem.c').read_text().split('static void draw(')[1].split('static int parse')[0]
assert 'fflush(stdout)' in (apps/'mb.c').read_text().split('static void draw(')[1].split('int main(')[0]
clang=shutil.which('clang')
if clang:
    for app in buffered:
        subprocess.run([clang,'--target=armv6m-none-eabi','-mcpu=cortex-m0plus','-mthumb','-Oz',
                       '-fsyntax-only','-I'+str(root/'userland/lib'),str(apps/f'{app}.c')],check=True)
print('PASS: buffered screen output, flush boundaries and ARM syntax')
