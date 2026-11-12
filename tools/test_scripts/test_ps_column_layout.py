#!/usr/bin/env python3
"""Host-execute ps using deliberately fragmented /proc/tasks reads."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
source=(root/'userland/apps/ps.c').read_text()
for header in ('#include "unistd.h"','#include "fcntl.h"','#include "stdio.h"'):
    source=source.replace(header,'')
scaffold=r'''
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
static const char records[] =
"PID NAME STATE TTY SLOT MEMORY SP PPID DISPATCH CPU_TICKS NET_US CON_US\n"
"0 [idle] READY - - - - 0 13288 132053 5082669 4787418\n"
"1 sh BLOCKED ttyV0 1 2000E000-20013FFF 20013A60 0 27 13 0 0\n"
"2 httpd BLOCKED ttyV0 2 20014000-20019FFF 20019E90 1 1 0 0 0\n"
"8 ps READY ttyV0 3 2001A000-2001FFFF 2001FE60 1 3 3 0 0\n";
static unsigned pos;
static int mock_open(const char *path,int flags) {(void)flags;return strcmp(path,"/proc/tasks")?-1:4;}
static int mock_read(int fd,void *buf,unsigned n) {(void)fd;if(pos>=sizeof(records)-1)return 0;
    if(n>13)n=13;
    if(n>sizeof(records)-1-pos)n=(unsigned)(sizeof(records)-1-pos);
    memcpy(buf,records+pos,n);pos+=n;return (int)n;}
static int mock_close(int fd) {(void)fd;return 0;}
#define open mock_open
#define read mock_read
#define close mock_close
#define main piconix_ps_main
'''
with tempfile.TemporaryDirectory() as d:
    c=Path(d)/'test.c'
    c.write_text(scaffold+source+'\n#undef main\nint main(void) {return piconix_ps_main(1,0);}\n')
    exe=Path(d)/'test'
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
    result=subprocess.run([str(exe)],capture_output=True,text=True,check=True)
    lines=result.stdout.splitlines()
    assert len(lines)==5,lines
    assert all(len(line)<=79 for line in lines),[(len(s),s) for s in lines]
    assert lines[1].split()==['0','[idle]','READY','-','-','0','13288','132053'],lines[1]
    assert lines[2].split()==['1','sh','BLOCKED','ttyV0','1','0','27','13'],lines[2]
    assert lines[3].split()[1]=='httpd' and lines[4].split()[1]=='ps'
    assert 'DISPATCH' in lines[0]
    print('PASS: ps compact columns, fragmented reads, idle and task counters')
