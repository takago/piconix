#!/usr/bin/env python3
"""Check fd's sparse seek index and fallback without Pico hardware."""
from pathlib import Path
import subprocess
import tempfile
import shutil

root = Path(__file__).resolve().parents[2]
src = root / 'userland/apps/fd.c'
cc = shutil.which('clang') or shutil.which('gcc')
with tempfile.TemporaryDirectory() as temporary:
    t = Path(temporary)
    (t / 'harness.c').write_text(r'''
#define main fd_program_main
#include "FD_SOURCE"
#undef main
static char fake[40000];
static int length, offset;
static int bytes_read;
static int seek_calls;
static int allow_seek=1;
int open(const char *p,int flags){(void)flags;offset=0;return !strcmp(p,"/proc/sym/kernel")?8:-1;}
int close(int fd){(void)fd;return 0;}
int read(int fd,void *buf,size_t n){
  if(fd!=8)return -1;
  if(offset>=length)return 0;
  if(n>(size_t)(length-offset))n=(size_t)(length-offset);
  memcpy(buf,fake+offset,n);offset+=(int)n;bytes_read+=(int)n;return (int)n;
}
long lseek(int fd,long pos,int whence){
  seek_calls++;
  if(fd!=8 || !allow_seek)return -1;
  long n=whence==SEEK_CUR?offset+pos:whence==SEEK_END?length+pos:pos;
  if(n<0||n>length)return -1;
  return offset=(int)n;
}
int main(void){
  for(int i=0;i<1000;i++)length+=snprintf(fake+length,sizeof fake-length,"symbol %04d value\n",i);
  strcpy(path,"/proc/sym/kernel");rows=24;cols=80;
  reset_view_index();view_top=400;
  if(view_load()<0||!strstr(file_data,"symbol 0400"))return 1;
  if(indexed_lines<20)return 2;
  bytes_read=0;seek_calls=0;
  view_top=417;
  if(view_load()<0||!strstr(file_data,"symbol 0417"))return 3;
  if(!seek_calls||bytes_read>1300)return 4;
  /* Reopen to view an earlier section using an established checkpoint. */
  bytes_read=0;view_top=80;
  if(view_load()<0||!strstr(file_data,"symbol 0080")||bytes_read>1300)return 5;
  /* Older kernels without lseek still work. */
  allow_seek=0;view_top=500;
  if(view_load()<0||!strstr(file_data,"symbol 0500"))return 6;
  return 0;
}
'''.replace('FD_SOURCE',str(src)))
    subprocess.run([cc, '-DPICONIX_HOST_TEST', '-O0', '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections',
                    '-I',str(root/'userland/lib'), '-I',str(root/'include'),
                    str(t/'harness.c'), '-o',str(t/'test')],check=True)
    subprocess.run([str(t/'test')],check=True)
print('PASS: fd seek checkpoints, reverse scrolling, no-lseek fallback')
