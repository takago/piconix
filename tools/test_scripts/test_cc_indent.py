#!/usr/bin/env python3
"""Verify cc's assembly layout and acceptance by PicoNIX as."""
from pathlib import Path
import subprocess
import tempfile
import json

root = Path(__file__).resolve().parents[2]
includes = ['-I' + str(root/'userland/apps'), '-I' + str(root/'userland/lib')]
program = '''int add(int a,int b){return a+b;}
int main(void){write(1,"HI\\n",3);return add(1,2);}'''
with tempfile.TemporaryDirectory() as d:
    t=Path(d)
    (t/'cc_host.c').write_text('''#define main cc_app_main
#include "cc.c"
#undef main
int main(int argc, char **argv){strcpy(source,PROGRAM);if(compile())return 1;
FILE *f=fopen(argv[1],"wb");if(!f)return 2;
fwrite(output,1,used,f);fclose(f);return 0;}'''.replace('PROGRAM',json.dumps(program)))
    (t/'as_host.c').write_text('''#define main asm_app_main
#include "asm.c"
#undef main
int main(int argc,char **argv){FILE *f=fopen(argv[1],"rb");if(!f)return 3;
size_t n=fread(src,1,MAX_SRC+1,f);fclose(f);if(n>MAX_SRC)return 4;
src[n]=0;int size=compile(0);if(size<=0||compile(1)!=size)return 1;return 0;}''')
    subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/'cc_host.c'),'-o',str(t/'cc_host')],check=True)
    subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/'as_host.c'),'-o',str(t/'as_host')],check=True)
    asm=t/'test.s'
    subprocess.run([str(t/'cc_host'),str(asm)],check=True)
    lines=asm.read_text().splitlines()
    assert any(x.startswith('  push {r4, lr}') for x in lines)
    assert any(x.startswith('f_add:') for x in lines)
    assert any(x.startswith('.align 2') for x in lines)
    assert any(x.startswith('.ascii "HI') for x in lines)
    for line in lines:
        if not line: continue
        if line[0]=='.' or line.endswith(':'):
            assert not line[0].isspace(), repr(line)
        else:
            assert line.startswith('  '), repr(line)
    subprocess.run([str(t/'as_host'),str(asm)],check=True)
print('PASS: cc assembly formatting, assembler parsing (not CPU execution)')
