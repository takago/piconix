#!/usr/bin/env python3
"""Host-level tests for the small 64-byte FILE implementation."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
code=r'''
#include "stdio.h"
#include "string.h"
#include "fcntl.h"
static unsigned char disk[512], terminal[512];
static int dlen,dpos,tlen,opened;
static const char *console="7 8\nmore\n";
static int cin;
int open(const char *path,int flags){
    if(strcmp(path,"/tmp/test"))return -1;
    opened=1;dpos=(flags&O_APPEND)?dlen:0;
    if(flags&O_TRUNC)dlen=0;
    return 8;
}
int close(int fd){if(fd!=8)return -1;opened=0;return 0;}
int write(int fd,const void *buf,size_t n){
    const unsigned char *b=buf;
    if(fd==8){for(size_t i=0;i<n;i++){disk[dpos++]=b[i];if(dpos>dlen)dlen=dpos;}return (int)n;}
    if(fd==1){for(size_t i=0;i<n;i++)terminal[tlen++]=b[i];return (int)n;}
    return (int)n;
}
int read(int fd,void *buf,size_t n){
    unsigned char *b=buf;
    if(fd==8){size_t i=0;while(i<n&&dpos<dlen)b[i++]=disk[dpos++];return (int)i;}
    if(fd==0){size_t i=0;while(i<n&&console[cin])b[i++]=console[cin++];return (int)i;}
    return -1;
}
int msleep(unsigned n){(void)n;return 0;}
#define CHECK(x) do{if(!(x))return __LINE__;}while(0)
int main(void){
    char result[130];
    FILE *f=fopen("/tmp/test","wb");CHECK(f);
    CHECK(fwrite("hello",1,5,f)==5 && dlen==0);
    CHECK(fprintf(f," %d",42)==3 && dlen==0);
    CHECK(fflush(f)==0 && dlen==8);
    CHECK(fclose(f)==0 && !opened);
    f=fopen("/tmp/test","rb");CHECK(f);
    CHECK(fread(result,1,8,f)==8);result[8]=0;
    CHECK(strcmp(result,"hello 42")==0);
    CHECK(fread(result,1,1,f)==0 && feof(f));
    CHECK(fclose(f)==0);
    f=fopen("/tmp/test","a");CHECK(f);
    CHECK(fwrite("!",1,1,f)==1 && fclose(f)==0);
    CHECK(dlen==9 && disk[8]=='!');
    CHECK(printf("Prompt: ")==8 && tlen==0);
    int a,b;CHECK(scanf("%d %d",&a,&b)==2 && a==7 && b==8);
    CHECK(tlen==8 && getchar()=='\n');
    CHECK(fgets(result,sizeof(result),stdin)==result && strcmp(result,"more\n")==0);
    CHECK(fgets(result,sizeof(result),stdin)==0 && feof(stdin));
    CHECK(printf("Hello\n")==6 && tlen==14);
    CHECK(printf("tail")==4 && tlen==14);
    CHECK(fflush(0)==0 && tlen==18);
    f=fopen("/tmp/test","r");CHECK(f);
    FILE *extra[3];for(int i=0;i<3;i++){extra[i]=fopen("/tmp/test","r");CHECK(extra[i]);}
    CHECK(fopen("/tmp/test","r")==0);
    CHECK(fclose(f)==0);
    for(int i=0;i<3;i++)CHECK(fclose(extra[i])==0);
    return 0;
}
'''
with tempfile.TemporaryDirectory() as d:
    src=Path(d)/'t.c';out=Path(d)/'t';src.write_text(code)
    subprocess.run(['cc','-O2','-fno-builtin','-I',str(root/'userland/lib'),str(root/'userland/lib/libc.c'),str(src),'-o',str(out)],check=True)
    subprocess.run([str(out)],check=True)
print('PASS: 64-byte FILE streams, flush, input, open/close, EOF')
