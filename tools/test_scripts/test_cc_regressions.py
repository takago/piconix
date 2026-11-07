#!/usr/bin/env python3
"""Regression tests for cc/as: successful and rejected C sources, binary encoding.

These tests check compilation and assembly, NOT execution on RP2040.
"""
from pathlib import Path
import json
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
includes = ['-I' + str(root / 'userland/apps'), '-I' + str(root / 'userland/lib')]
positive = [
    'int main(void) { char buf[32]; int n; n=read(0,buf,sizeof(buf)); if(n>0){write(1,buf,n);} return 0; }',
    'int main(void) { int x=3; int y=5; int z=(x+y)*2; if(z==16){write(1,"OK\\n",3);} return z; }',
    'int main(void) { char msg[6]="HELLO"; int i=0; while(i<5){write(1,msg,i+1);i=i+1;}return 0; }',
    'int main(void) { int a[4]; a[0]=7; a[1]=a[0]*2; if(a[1]>10){write(1,"YES",3);}return 0; }',
]
negative = [
    'int main(void) { char b[0]; return 0; }',
    'int main(void) { char b[65]; return 0; }',
    'int main(void) { int a; int a; return 0; }',
    'int main(void) { int a=1; unknown=2; return 0; }',
    'int main(void) { char b[2]="TOO LONG"; return 0; }',
]
with tempfile.TemporaryDirectory() as directory:
    tmp = Path(directory)
    csrc = '''#define main cc_app_main
#include "cc.c"
#undef main
int main(int argc,char **argv) {
 if(argc!=3)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 size_t n=fread(source,1,SOURCE_MAX+1,f);fclose(f);
 if(n>SOURCE_MAX)return 4;source[n]=0;
 if(compile())return 1;
 f=fopen(argv[2],"wb");if(!f)return 5;
 fwrite(output,1,used,f);fclose(f);return 0;
}'''
    asrc = '''#define main asm_app_main
#include "asm.c"
#undef main
int main(int argc,char **argv) {
 if(argc!=3)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 size_t n=fread(src,1,MAX_SRC+1,f);fclose(f);
 if(n>MAX_SRC)return 4;src[n]=0;
 int size=compile(0);if(size<=0||compile(1)!=size)return 1;
 f=fopen(argv[2],"wb");if(!f)return 5;
 fwrite(code,1,(size_t)size,f);fclose(f);return 0;
}'''
    (tmp/'cc_host.c').write_text(csrc)
    (tmp/'as_host.c').write_text(asrc)
    for source, target in [('cc_host.c','cc_host'),('as_host.c','as_host')]:
        subprocess.run(['cc','-std=c11','-DPICONIX_HOST_TEST',*includes,str(tmp/source),'-o',str(tmp/target)],check=True)
    for i, program in enumerate(positive):
        source, asm, binary = (tmp/f'{i}{suffix}' for suffix in ('.c','.s','.bin'))
        source.write_text(program)
        subprocess.run([str(tmp/'cc_host'),str(source),str(asm)],check=True)
        subprocess.run([str(tmp/'as_host'),str(asm),str(binary)],check=True)
        code = binary.read_bytes()
        assert code and len(code)%2==0, 'invalid Thumb image length'
        assert b'\x00\xdf' in code, 'missing SVC #0 instruction'
        print(f'PASS: positive {i}: {len(code)} code bytes, SVC present')
    for i, program in enumerate(negative):
        source, asm = tmp/f'bad{i}.c', tmp/f'bad{i}.s'
        source.write_text(program)
        result = subprocess.run([str(tmp/'cc_host'),str(source),str(asm)],capture_output=True)
        assert result.returncode != 0, f'invalid program accepted: {i}'
        print(f'PASS: negative {i} rejected')
print('PASS: compile/assemble regression suite (not a CPU execution test)')
