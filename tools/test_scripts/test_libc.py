#!/usr/bin/env python3
"""Host unit checks for PIC-safe user libc (no Pico SDK required)."""
import pathlib
import subprocess
import tempfile
import shutil

root = pathlib.Path(__file__).resolve().parents[2]
cc = shutil.which('cc')
if not cc:
    raise SystemExit('Missing native C compiler')
source = r'''
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stddef.h>
int write(int fd,const void *data,size_t n){(void)fd;(void)data;return (int)n;}
static const char *input="";
static unsigned input_pos;
int read(int fd,void *data,size_t n){
    (void)fd;(void)n;
    if(!input[input_pos])return 0;
    *(char *)data=input[input_pos++];return 1;
}
int open(const char *path,int flags){(void)path;(void)flags;return -1;}
int close(int fd){(void)fd;return 0;}
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
    CHECK(strtoul("4294967295!",&end,10)==4294967295ul && *end=='!');
    CHECK(strtoul("0xff!",&end,0)==255ul && *end=='!');
    CHECK(memchr("abcd",'c',4)==(const char *)"abcd"+2 ||
          (unsigned char)*(char *)memchr("abcd",'c',4)=='c');
    CHECK(strtol("0x10!",&end,0)==16 && *end=='!');
    CHECK(strtol("-2147483648",&end,10)==(-2147483647L-1) && *end==0);
    CHECK(snprintf(buf,sizeof(buf),"%d %u %x %s %%",-12,34u,0x2au,"hi")==14);
    CHECK(strcmp(buf,"-12 34 2a hi %")==0);
    CHECK(snprintf(buf,5,"abcdef")==6 && strcmp(buf,"abcd")==0);
    CHECK(snprintf((char *)0,0,"%d",123)==3);
    CHECK(sprintf(buf,"%X %c",255u,'Z')==4 && strcmp(buf,"FF Z")==0);
    int a=0,b=0,c=0;unsigned hex=0;
    char token[12];
    CHECK(sscanf("3 4", "%d %d", &a, &b)==2 && a==3 && b==4);
    CHECK(sscanf("  -12 0xff next", "%d %x %11s", &a, &hex, token)==3);
    CHECK(a==-12 && hex==255u && strcmp(token,"next")==0);
    CHECK(sscanf("0x10 077 12", "%i %i %i", &a, &b, &c)==3);
    CHECK(a==16 && b==63 && c==12);
    CHECK(sscanf("q", "%d", &a)==0);
    CHECK(sscanf("", "%d", &a)==EOF);
    CHECK(sscanf("ab", "%2c", token)==1 && token[0]=='a' && token[1]=='b');
    CHECK(sscanf("12 34", "%*d %d", &a)==1 && a==34);
    CHECK(sscanf("% 9", "%% %d", &a)==1 && a==9);
    input="3 4\nHELLO\n";input_pos=0;
    CHECK(scanf("%d", &a)==1 && a==3);
    CHECK(scanf("%d", &a)==1 && a==4);
    CHECK(getchar()=='\n');
    CHECK(fgets(token,sizeof(token),stdin)==token && strcmp(token,"HELLO\n")==0);
    CHECK(scanf("%d", &a)==EOF);
    input="abc 5";input_pos=0;
    CHECK(scanf("%d", &a)==0 && getchar()=='a');
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='piconix-libc-') as temp:
    p = pathlib.Path(temp)
    (p/'test.c').write_text(source)
    subprocess.run([cc, '-O2', '-fno-builtin', '-I', str(root/'userland/lib'),
                    str(root/'userland/lib/libc.c'), str(p/'test.c'), '-o', str(p/'test')], check=True)
    subprocess.run([str(p/'test')], check=True)
print('PASS: libc string/memory/integer/formatting host tests')
