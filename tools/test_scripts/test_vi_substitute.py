#!/usr/bin/env python3
"""Exercise PicoNIX vi's literal substitution and its 2 KiB bound on a host."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
cc = shutil.which('gcc') or shutil.which('clang')
assert cc, 'host C compiler required'
source = r'''
#define main piconix_app_main
#include "vi.c"
#undef main
int main(void) {
    char cmd1[]="%s/1000/100/g";
    memcpy(data,"1000 1000\n",10);len=10;pos=4;changed=0;
    if(substitute(cmd1)!=2 || len!=8 || memcmp(data,"100 100\n",8) || !changed)return 1;
    char cmd2[]="%s/100/ABCD/g";
    if(substitute(cmd2)!=2 || len!=10 || memcmp(data,"ABCD ABCD\n",10))return 2;
    char bad[]="%s//x/g";
    if(substitute(bad)!=-1)return 3;
    char invalid[]="%s/a/b/";
    if(substitute(invalid)!=-1)return 4;
    len=CAP;memset(data,'a',len);changed=0;
    char overflow[]="%s/a/aa/g";
    if(substitute(overflow)!=-2 || len!=CAP || !(!changed))return 5;
    char delete_cmd[]="%s/a//g";
    if(substitute(delete_cmd)!=CAP || len!=0)return 6;
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    test = Path(d) / 'test.c'
    binary = Path(d) / 'test'
    test.write_text(source)
    subprocess.run([cc,'-std=c11','-O1','-DPICONIX_HOST_TEST','-ffunction-sections','-fdata-sections',
                    '-Wl,--gc-sections','-I',str(root/'userland/apps'),'-I',str(root/'userland/lib'),
                    str(test),'-o',str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS: vi literal substitution, no match/invalid syntax, bounds, deletion')
