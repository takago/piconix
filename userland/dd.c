/* SPDX-License-Identifier: GPL-3.0-or-later */
/* PicoNIX streaming block copy. count and skip/seek are in blocks. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>
static unsigned char buf[256];
static int number(const char *p,uint32_t *out){
 uint32_t x=0,base=10; if(p[0]=='0'&&(p[1]=='x'||p[1]=='X')){base=16;p+=2;}
 if(!*p)return -1;
 while(*p){unsigned d;char c=*p++;if(c>='0'&&c<='9')d=c-'0';else if(c>='a'&&c<='f')d=c-'a'+10;else if(c>='A'&&c<='F')d=c-'A'+10;else return -1;
  if(d>=base||x>(0xffffffffu-d)/base)return -1;x=x*base+d;}
 *out=x;return 0;
}
static void usage(void){puts("usage: dd [if=FILE|-] [of=FILE|-] [bs=1..256] [count=N] [skip=N] [seek=N]");puts("  /dev/mem uses absolute address as byte offset; values can be hex (0x...)");}
int main(int argc,char **argv){
 const char *in=0,*out=0;uint32_t bs=128,count=0,skip=0,seek=0;int limited=0;
 for(int i=1;i<argc;i++){
  const char *v=0;uint32_t *dst=0;
  if(!strncmp(argv[i],"if=",3)){in=argv[i]+3;continue;}
  if(!strncmp(argv[i],"of=",3)){out=argv[i]+3;continue;}
  if(!strncmp(argv[i],"bs=",3)){v=argv[i]+3;dst=&bs;}
  else if(!strncmp(argv[i],"count=",6)){v=argv[i]+6;dst=&count;limited=1;}
  else if(!strncmp(argv[i],"skip=",5)){v=argv[i]+5;dst=&skip;}
  else if(!strncmp(argv[i],"seek=",5)){v=argv[i]+5;dst=&seek;}
  else {usage();return 1;}
  if(number(v,dst)<0){usage();return 1;}
 }
 if(bs<1||bs>sizeof(buf)){usage();return 1;}
 int fi=(in && strcmp(in,"-"))?open(in,O_RDONLY):0;
 if(fi<0){puts("dd: cannot open input");return 1;}
 int fo=(out && strcmp(out,"-"))?open(out,O_WRONLY | (strcmp(out,"/dev/mem")?O_CREAT|O_TRUNC:0)):1;
 if(fo<0){puts("dd: cannot open output");if(fi!=0)close(fi);return 1;}
 uint32_t offs;
 if(skip){if(skip>0xffffffffu/bs){puts("dd: input offset overflow");goto fail;}offs=skip*bs;
  if(lseek(fi,(long)offs,SEEK_SET)<0){puts("dd: input not seekable");goto fail;}}
 if(seek){if(seek>0xffffffffu/bs){puts("dd: output offset overflow");goto fail;}offs=seek*bs;
  if(lseek(fo,(long)offs,SEEK_SET)<0){puts("dd: output not seekable");goto fail;}}
 /* Explicit count is required when reading /dev/mem to prevent open-ended scans. */
 if(in&&!strcmp(in,"/dev/mem")&&!limited){puts("dd: count= required for /dev/mem input");goto fail;}
 uint32_t blocks=0;
 while(!limited||blocks<count){int r=read(fi,buf,bs);if(r<0){puts("dd: read failed");goto fail;}if(!r)break;
  int done=0;while(done<r){int n=write(fo,buf+done,(unsigned)(r-done));if(n<=0){puts("dd: write failed");goto fail;}done+=n;}
  blocks++;}
 if(fi!=0)close(fi);if(fo!=1)close(fo);return 0;
fail: if(fi!=0)close(fi);if(fo!=1)close(fo);return 1;
}
