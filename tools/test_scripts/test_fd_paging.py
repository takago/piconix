#!/usr/bin/env python3
"""Host execution test of fd incremental /proc browsing."""
from pathlib import Path
import subprocess, tempfile, shutil
root = Path(__file__).resolve().parents[2]
src = root / 'userland' / 'apps' / 'fd.c'
cc = shutil.which('clang') or shutil.which('gcc')
with tempfile.TemporaryDirectory() as tmp:
    t = Path(tmp)
    (t/'fd.c').write_bytes(src.read_bytes())
    (t/'harness.c').write_text(r'''#define main fd_program_main
#include "fd.c"
#undef main
static char fake[28000];
static int length, offset;
int open(const char *p,int flags){(void)flags;return strcmp(p,"/proc/sym/kernel")==0?8:-1;}
int close(int fd){(void)fd;return 0;}
int read(int fd,void *p,size_t sz){
  if(fd!=8)return -1;
  if(offset>=length)return 0;
  if(sz>(size_t)(length-offset))sz=(size_t)(length-offset);
  memcpy(p,fake+offset,sz);offset+=(int)sz;return (int)sz;
}
int main(void){
  int n=0;
  for(int i=0;i<900;i++){
    n+=snprintf(fake+n,sizeof(fake)-n,"entry %03d symbol \n",i);
  }
  length=n;
  strcpy(path,"/proc/sym/kernel");
  rows=24;cols=80;
  int esc=0;
  if(key_event(27,&esc)!=0 || key_event('[',&esc)!=0 || key_event('6',&esc)!=0 || key_event('~',&esc)!=1006)return 5;
  esc=0;
  if(key_event(27,&esc)!=0 || key_event('[',&esc)!=0 || key_event('5',&esc)!=0 || key_event('~',&esc)!=1005)return 6;
  view_top=0;offset=0;
  int r=view_load();
  if(r<0||!strstr(file_data,"entry 000")||!view_more)return 1;
  view_top=100;offset=0;r=view_load();
  if(r<0||!strstr(file_data,"entry 100")||strstr(file_data,"entry 000")||!view_more)return 2;
  view_top=890;offset=0;r=view_load();
  if(r<0||!strstr(file_data,"entry 890")||view_more)return 3;
  view_top=900;offset=0;r=view_load();
  if(r!=0||view_more||view_lines!=0)return 4;
  return 0;
}
''')
    h = t/'harness.c'
    h.write_text(h.read_text().replace('#include "fd.c"', '#include "'+str(src)+'"'))
    subprocess.run([cc, '-DPICONIX_HOST_TEST','-O0','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-I',str(t),'-I',str(root/'userland/lib'),'-I',str(root/'include'),str(t/'harness.c'),'-o',str(t/'test')],check=True)
    subprocess.run([str(t/'test')],check=True)
print('PASS: fd views beyond 2 KiB including end of /proc-like stream')
