#!/usr/bin/env python3
"""Verify echo batches small output without changing escaped/quoted bytes."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = root / 'userland/apps/echo.c'
harness = r'''
#include <stddef.h>
#include <string.h>
#include <stdio.h>

int piconix_echo_main(int argc, char **argv);
static char buffer[1024];
static int written, calls, partial;
int write(int fd, const void *p, size_t n) {
    if (fd!=1 || written+(int)n>1024) return -1;
    if(partial && n>3) n=3;
    memcpy(buffer+written,p,n);
    written+=(int)n;
    calls++;
    return (int)n;
}
void msleep(unsigned ms) { (void)ms; }
static int check(char **args,int argc,const char *expect,int size,int max_calls,int short_write) {
    written=0;calls=0;partial=short_write;
    if(piconix_echo_main(argc,args)!=0 || written!=size || memcmp(buffer,expect,size) || calls>max_calls) {
        fprintf(stderr,"echo failure: size=%d expected=%d calls=%d max=%d\n",written,size,calls,max_calls);
        return 1;
    }
    return 0;
}
int main(void) {
    char *a[]={"echo","heart"};
    char *b[]={"echo","-n","heart"};
    char *c[]={"echo","a","b"};
    char *d[]={"echo","-e","'A\\nB'"};
    char *e[]={"echo","-ne","'\\x41\\033'"};
    if(check(a,2,"heart\n",6,1,0))return 1;
    if(check(b,3,"heart",5,1,0))return 1;
    if(check(c,3,"a b\n",4,1,0))return 1;
    if(check(d,3,"A\nB\n",4,1,0))return 1;
    if(check(e,3,"A\033",2,1,0))return 1;
    if(check(a,2,"heart\n",6,2,1))return 1;
    char longarg[262]; memset(longarg,'X',261);longarg[261]=0;
    char *f[]={"echo","-n",longarg};
    if(check(f,3,longarg,261,3,0))return 1;
    puts("PASS: echo batching, escape sequences, newline, partial writes");
    return 0;
}
'''
with tempfile.TemporaryDirectory() as td:
    td=Path(td)
    (td/'harness.c').write_text(harness)
    exe=td/'echo-test'
    cmd=['cc','-std=c99','-DPICONIX_HOST_TEST','-Dmain=piconix_echo_main',
         '-I'+str(root/'userland/lib'),'-I'+str(root/'include'),
         '-c',str(source),'-o',str(td/'echo.o')]
    subprocess.run(cmd,check=True)
    subprocess.run(['cc',str(td/'echo.o'),str(td/'harness.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
