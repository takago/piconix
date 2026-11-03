#!/usr/bin/env python3
"""Check common editor logic and both users of the shared PIC header."""
from pathlib import Path
import subprocess
import tempfile
import shutil

root = Path(__file__).resolve().parents[2]
editor = root / 'userland' / 'apps'
common = (editor / 'editor_common.h').read_text()
for name, gutter in [('edit', 4), ('vi', 0)]:
    src = (editor / f'{name}.c').read_text()
    assert '#include "editor_common.h"' in src
    assert f'#define EDITOR_GUTTER {gutter}' in src
    assert 'editor_start(argv[1],path)' in src
    assert 'static void measure_screen(' not in src  # one implementation
    assert '#define CAP 2048' in common

cc = shutil.which('gcc') or shutil.which('clang')
assert cc, 'C compiler required'
for name in ('edit', 'vi'):
    subprocess.run([cc, '-std=c11', '-fsyntax-only', '-Wall', '-Wextra', '-I', str(root/'userland/lib'),
                    str(editor / f'{name}.c')], check=True)

# Exercise the actual shared buffer/column operations on a host compiler. Other
# unused editor functions are dead-stripped to avoid simulating the Pico TTY.
source = r'''
#define EDITOR_NAME "edit"
#define EDITOR_GUTTER 4
#define EDITOR_TITLE "PicoNIX edit  "
#define EDITOR_HINT "test"
#include "editor_common.h"
int main(void){
    insert('A');insert('\t');insert('B');insert('\n');insert('Z');
    if(len!=5 || pos!=5 || line_no(pos)!=1 || col_no(3)!=5)return 1;
    if(position_col(0,4)!=2 || line_start(pos)!=4 || line_end(0)!=3)return 2;
    erase();if(len!=4 || pos!=4 || data[3]!='\n')return 3;
    pos=2;del();if(len!=3 || data[2]!='\n')return 4;
    len=CAP;pos=0;changed=0;insert('Q');
    if(len!=CAP || pos!=0 || changed!=0)return 5;
    return 0;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    f = Path(tmp) / 'editor_test.c'
    exe = Path(tmp) / 'editor_test'
    f.write_text(source)
    subprocess.run([cc, '-std=c11', '-O1', '-DPICONIX_HOST_TEST',
                    '-ffunction-sections', '-fdata-sections',
                    '-Wl,--gc-sections', '-I', str(editor), '-I', str(root / 'userland' / 'lib'), str(f), '-o', str(exe)],
                   check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: shared editor primitives, bounds and independent syntax')
