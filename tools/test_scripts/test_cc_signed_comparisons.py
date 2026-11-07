#!/usr/bin/env python3
"""Verify signed comparison code generation and Thumb-1 branch encodings.

This checks the emitted binary, not CPU execution.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
includes = ['-I' + str(root / 'userland/apps'), '-I' + str(root / 'userland/lib')]
program = '''int main(void) {
 int a = -1; int b = 1;
 if (a < b) { write(1, "L", 1); }
 if (a <= b) { write(1, "E", 1); }
 if (a > b) { write(1, "G", 1); }
 if (a >= b) { write(1, "H", 1); }
 return 0;
}'''
with tempfile.TemporaryDirectory() as d:
    t = Path(d)
    (t/'cc_host.c').write_text('''#define main cc_app_main
#include "cc.c"
#undef main
int main(int argc, char **argv) {
 if(argc!=2)return 2;
 strcpy(source, PROGRAM);
 if(compile())return 1;
 FILE *f=fopen(argv[1],"wb");if(!f)return 3;
 fwrite(output,1,used,f);fclose(f);return 0;
}'''.replace('PROGRAM', __import__('json').dumps(program)))
    (t/'as_host.c').write_text('''#define main asm_app_main
#include "asm.c"
#undef main
int main(int argc,char **argv){
 if(argc!=3)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 size_t n=fread(src,1,MAX_SRC+1,f);fclose(f);if(n>MAX_SRC)return 4;src[n]=0;
 int size=compile(0);if(size<=0||compile(1)!=size)return 1;
 f=fopen(argv[2],"wb");if(!f)return 5;
 fwrite(code,1,size,f);fclose(f);return 0;
}''')
    for name in ('cc_host','as_host'):
        subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(t/f'{name}.c'),'-o',str(t/name)],check=True)
    asm, binary = t/'signed.s', t/'signed.bin'
    subprocess.run([str(t/'cc_host'),str(asm)],check=True)
    content = asm.read_text()
    for op in ('bge','bgt','ble','blt'):
        assert op+' ' in content, f'missing signed branch {op}'
    subprocess.run([str(t/'as_host'),str(asm),str(binary)],check=True)
    words = [int.from_bytes(binary.read_bytes()[i:i+2], 'little') for i in range(0,binary.stat().st_size,2)]
    for condition in (10,11,12,13):
        assert any((w & 0xff00)==(0xd000|(condition<<8)) for w in words), f'missing Thumb condition {condition}'
    print('PASS: signed comparison branches encoded correctly (not CPU execution)')
