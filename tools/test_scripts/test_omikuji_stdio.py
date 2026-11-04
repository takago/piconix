#!/usr/bin/env python3
"""Host regression: omikuji arguments and repeated buffered stdin tokens."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
app = root / 'userland/apps/omikuji.c'
lib = root / 'userland/lib'
stub = r'''
#include "stdio.h"
#include "string.h"
#include "fcntl.h"
#include "unistd.h"
static const char *input;
static int offset, ticks, olen;
static char output[8192];
void set_input(const char *s){input=s;offset=0;ticks=0;olen=0;__piconix_stdin.pos=0;__piconix_stdin.len=0;__piconix_stdin.saved_count=0;__piconix_stdin.flags=1;}
int get_ticks(void){return ticks;}
const char *get_output(void){output[olen]=0;return output;}
int open(const char *path,int flags){(void)flags;return strcmp(path,"/dev/rand")==0?7:-1;}
int close(int fd){return fd==7?0:-1;}
int read(int fd,void *buf,size_t size){
 if(fd==7){if(size<4)return -1;memset(buf,0,4);return 4;}
 if(fd!=0)return -1;
 size_t n=0;while(n<size&&input[offset])((char *)buf)[n++]=input[offset++];
 return (int)n;
}
int write(int fd,const void *buf,size_t size){
 if(fd!=1&&fd!=2)return -1;
 if(olen+(int)size>=(int)sizeof(output))return -1;
 memcpy(output+olen,buf,size);olen+=(int)size;return (int)size;
}
int msleep(unsigned n){if(n==1000)ticks++;return 0;}
'''
main = r'''
#include "stdio.h"
#include "string.h"
extern int omikuji_main(int,char **);
extern void set_input(const char *);
extern int get_ticks(void);
extern const char *get_output(void);
#define CHECK(x) do {if(!(x))return __LINE__;}while(0)
int main(void){
 char *none[]={"omikuji",0};
 char *short_wait[]={"omikuji","2",0};
 char *interactive[]={"omikuji","-i",0};
 char *invalid[]={"omikuji","31",0};
 set_input("");CHECK(omikuji_main(1,none)==0);fflush(0);CHECK(get_ticks()==10);
 set_input("");CHECK(omikuji_main(2,short_wait)==0);fflush(0);CHECK(get_ticks()==2);
 set_input("");CHECK(omikuji_main(2,invalid)==1);fflush(0);CHECK(get_ticks()==0);
 set_input("3 4\nq\n\n2\n");CHECK(omikuji_main(2,interactive)==0);fflush(0);
 CHECK(get_ticks()==9);
 CHECK(strstr(get_output(),"3...2...1...")!=0);
 CHECK(strstr(get_output(),"4...3...2...1...")!=0);
 set_input("q\n\nabc 1\n");CHECK(omikuji_main(2,interactive)==0);fflush(0);CHECK(get_ticks()==1);
 return 0;
}
'''
with tempfile.TemporaryDirectory() as t:
    d=Path(t)
    (d/'stub.c').write_text(stub)
    (d/'test.c').write_text(main)
    cmd=['cc','-O2','-fno-builtin','-DPICONIX_HOST_TEST','-I',str(lib)]
    subprocess.run(cmd+['-Dmain=omikuji_main','-c',str(app),'-o',str(d/'app.o')],check=True)
    subprocess.run(cmd+[str(lib/'libc.c'),str(d/'stub.c'),str(d/'test.c'),str(d/'app.o'),'-o',str(d/'test')],check=True)
    subprocess.run([str(d/'test')],check=True)
print('PASS: omikuji default, arguments, repeated numeric input, invalid tokens, EOF')
