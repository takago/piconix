#!/usr/bin/env python3
"""Host unit checks for PIC-safe user libc (no Pico SDK required)."""
import pathlib
import subprocess
import tempfile
import shutil

root = pathlib.Path(__file__).resolve().parents[1]
cc = shutil.which('cc')
if not cc:
    raise SystemExit('Missing native C compiler')
source = r'''
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stddef.h>
int write(int fd,const void *data,size_t n){(void)fd;(void)data;return (int)n;}
int read(int fd,void *data,size_t n){(void)fd;(void)data;(void)n;return -1;}
int msleep(unsigned n){(void)n;return 0;}
#define CHECK(x) do {if(!(x))return __LINE__;}while(0)
int main(void){
    char buf[64];char tmp[16];char *end;
    CHECK(strlen("abc")==3);
    strcpy(tmp,"ab");strcat(tmp,"cd");CHECK(strcmp(tmp,"abcd")==0);
    CHECK(strncmp("abc","abd",2)==0);
    CHECK(strchr(tmp,'c')==tmp+2);
    CHECK(strstr(tmp,"bc")==tmp+1);
    memcpy(buf,"abc",4);memmove(buf+1,buf,4);CHECK(strcmp(buf,"aabc")==0);
    memset(buf,'x',3);CHECK(memcmp(buf,"xxx",3)==0);
    CHECK(atoi("-123")==-123);
    CHECK(strtol("0x10!",&end,0)==16 && *end=='!');
    CHECK(strtol("-2147483648",&end,10)==(-2147483647L-1) && *end==0);
    CHECK(snprintf(buf,sizeof(buf),"%d %u %x %s %%",-12,34u,0x2au,"hi")==14);
    CHECK(strcmp(buf,"-12 34 2a hi %")==0);
    CHECK(snprintf(buf,5,"abcdef")==6 && strcmp(buf,"abcd")==0);
    CHECK(snprintf((char *)0,0,"%d",123)==3);
    CHECK(sprintf(buf,"%X %c",255u,'Z')==4 && strcmp(buf,"FF Z")==0);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='piconix-libc-') as temp:
    p = pathlib.Path(temp)
    (p/'test.c').write_text(source)
    subprocess.run([cc, '-O2', '-fno-builtin', '-I', str(root/'ram_apps'),
                    str(root/'ram_apps/libc.c'), str(p/'test.c'), '-o', str(p/'test')], check=True)
    subprocess.run([str(p/'test')], check=True)
print('PASS: libc string/memory/integer/formatting host tests')
