#!/usr/bin/env python3
"""Check fd's parent navigation and the input-mode handoff to edit."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
src = root / 'userland' / 'apps' / 'fd.c'
code = src.read_text()
assert 'original_flags & ~O_NONBLOCK' in code  # child must inherit blocking stdin
assert 'fcntl(0,F_SETFL,original_flags|O_NONBLOCK)' in code
assert 'if(!strcmp(entries[selected].name,".."))' in code
assert 'if(!strcmp(e.name,".")) continue;' in code
assert 'static const char *notice;' in code  # no address-valued static initializer
assert "key=='q' ||key==27||key==8||key==127||key==1004" in code  # Left closes viewer
assert "key=='\\r'||key=='\\n'||key==1003" in code  # Right opens selection
cc = shutil.which('gcc') or shutil.which('clang')
assert cc, 'C compiler required'
with tempfile.TemporaryDirectory() as t:
    t = Path(t)
    harness = t / 'test.c'
    harness.write_text('''
#define main fd_program_main
#include "fd.c"
#undef main
int main(void) {
    strcpy(current,"/tmp");parent_dir();if(strcmp(current,"/"))return 1;
    strcpy(current,"/dev/gpio");parent_dir();if(strcmp(current,"/dev"))return 2;
    parent_dir();if(strcmp(current,"/"))return 3;
    parent_dir();if(strcmp(current,"/"))return 4;
    int state=0;
    if(key_event(27,&state)!=0 || key_event('[',&state)!=0 || key_event('D',&state)!=1004)return 5;
    state=0;
    if(key_event(27,&state)!=0 || key_event('[',&state)!=0 || key_event('C',&state)!=1003)return 6;
    return 0;
}
''')
    cmd = [cc, '-DPICONIX_HOST_TEST', '-O0', '-ffunction-sections',
           '-fdata-sections', '-Wl,--gc-sections',
           '-I', str(root/'userland/apps'), '-I', str(root/'userland/lib'),
           '-I', str(root/'include'), str(harness), '-o', str(t/'test')]
    subprocess.run(cmd, check=True)
    subprocess.run([str(t/'test')], check=True)
print('PASS: fd parent navigation and editor input handoff checks')
