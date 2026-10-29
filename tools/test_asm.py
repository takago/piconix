#!/usr/bin/env python3
"""Test the native Thumb subset encoder without running an RP2040 firmware image."""
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[1]
source=r'''
#define main asm_app_main
#include "asm.c"
#undef main
int open(const char *p,int f){(void)p;(void)f;return -1;}
int close(int fd){(void)fd;return 0;}
int read(int fd,void *b,size_t n){(void)fd;(void)b;(void)n;return -1;}
int write(int fd,const void *b,size_t n){(void)fd;(void)b;return (int)n;}
int puts(const char *s){(void)s;return 0;}
int printf(const char *s,...){(void)s;return 0;}
int main(void){
    strcpy(src,"movs r0, #1\nsvc #0\nbx lr\n");
    if(compile(0)!=6||compile(1)!=6)return 1;
    if(code[0]!=1||code[1]!=0x20||code[2]!=0||code[3]!=0xdf||code[4]!=0x70||code[5]!=0x47)return 2;
    nlabels=0;strcpy(src,"movs r0, #9\nmovs r1, #1\nadr r2, msg\nmovs r3, #6\nsvc #0\nbx lr\n.align 2\nmsg:\n.ascii \"Hello\\n\"\n");
    if(compile(0)!=18||compile(1)!=18)return 3;
    if(code[4]!=1||code[5]!=0xa2||code[12]!='H'||code[17]!='\n')return 4;
    nlabels=0;strcpy(src,"again:\nsubs r0, #1\nbne again\nbx lr\n");
    if(compile(0)!=6||compile(1)!=6)return 5;
    if(code[2]!=0xfd||code[3]!=0xd1)return 6;
    nlabels=0;strcpy(src,"push {r4, lr}\nbl done\nmov r2, r3\nstrb r2, [r1, #1]\nldr r4, [r1, #4]\ndone:\npop {r4, pc}\n");
    if(compile(0)!=14 || compile(1)!=14)return 7;
    if(code[0]!=0x10 || code[1]!=0xb5 || code[2]!=0 || code[3]!=0xf0 || code[4]!=3 || code[5]!=0xf8)return 8;
    if(code[6]!=0x1a || code[7]!=0x46 || code[8]!=0x4a || code[9]!=0x70 || code[10]!=0x4c || code[11]!=0x68)return 9;
    if(code[12]!=0x10 || code[13]!=0xbd)return 10;
    nlabels=0;strcpy(src,".align 2\n.word 1234\n.byte 7\n.space 3\n");
    if(compile(0)!=8||compile(1)!=8)return 11;
    if(code[0]!=0xd2||code[1]!=4||code[4]!=7||code[7]!=0)return 12;
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    test=Path(d)/'test.c';test.write_text(source)
    exe=Path(d)/'check'
    subprocess.run(['cc','-std=c11','-I',str(root/'userland'),str(test),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: tiny assembler encodes SVC, ADR, data, and backward branches')

# Assemble each bundled teaching example with the same native encoder.
import json
for asm_source in sorted((root/'examples'/'asm').glob('*.s')):
    data=asm_source.read_text()
    assert len(data.encode())<=1024, f'{asm_source.name}: source exceeds /tmp limit'
    probe='''#define main asm_app_main
#include "asm.c"
#undef main
int open(const char*p,int f){(void)p;(void)f;return -1;}
int close(int f){(void)f;return 0;}
int read(int f,void*b,size_t n){(void)f;(void)b;(void)n;return -1;}
int write(int f,const void*b,size_t n){(void)f;(void)b;return (int)n;}
int puts(const char*s){(void)s;return 0;}
int printf(const char*s,...){(void)s;return 0;}
int main(void){strcpy(src, SOURCE);int a=compile(0);return a>0 && compile(1)==a ? 0:1;}
'''.replace('SOURCE',json.dumps(data))
    with tempfile.TemporaryDirectory() as d:
        unit=Path(d)/'example.c';unit.write_text(probe)
        exe=Path(d)/'example'
        subprocess.run(['cc','-std=c11','-I',str(root/'userland'),str(unit),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
print('PASS: all four assembly examples fit /tmp and encode successfully')
