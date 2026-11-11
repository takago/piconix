#!/usr/bin/env python3
"""Build top's actual task parser and verify common-interval CPU accounting."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
source=root/'userland/apps/top.c'
with tempfile.TemporaryDirectory() as tmp:
    f=Path(tmp)/'main.c'
    f.write_text(f'''#define PICONIX_HOST_TEST 1
#define main piconix_top_main
#include "{source}"
#undef main
int main(void){{
  char prev[TOP_ROWS][TOP_COLS+1]={{0}};
  unsigned v[5]={{0}};
  const char *start="PID NAME STATE TTY SLOT MEMORY SP PPID DISPATCH CPU_TICKS NET_US CON_US\\n"
        " 0 [idle] READY - - - - 0 30 9000 100000 50000\\n"
        " 1 sh BLOCKED console 1 2000E000-20013FFF 20013ABC 0 50 500 0 0\\n"
        " 4 top READY console 4 20020000-20025FFF 20025ABC 1 60 500 0 0\\n";
  const char *next="PID NAME STATE TTY SLOT MEMORY SP PPID DISPATCH CPU_TICKS NET_US CON_US\\n"
        " 0 [idle] READY - - - - 0 40 9900 130000 70000\\n"
        " 1 sh BLOCKED console 1 2000E000-20013FFF 20013ABC 0 50 500 0 0\\n"
        " 4 top READY console 4 20020000-20025FFF 20025ABC 1 70 600 0 0\\n";
  if(task_lines(prev,start,1000,v)!=3)return 1;
  if(task_lines(prev,next,2000,v)!=3)return 2;
  /* 100ms task, 30ms net, 20ms console, 850ms pure idle. */
  if(v[0]!=850 || v[1]!=150 || v[2]!=20 || v[3]!=100 || v[4]!=30)return 3;
  if(!strstr(prev[8],"85.0%") || !strstr(prev[10],"10.0%"))return 4;
  if(!strstr(prev[8],"[idle]") || !strstr(prev[9],"sh"))return 5;
  if(task_lines(prev,"PID NAME\\n",3000,v)!=0)return 6;
  if(prev[8][0]||prev[9][0]||prev[10][0])return 7;
  return 0;
}}
''')
    binary=Path(tmp)/'test'
    subprocess.run(['gcc','-std=c11','-O2','-ffunction-sections','-fdata-sections',
                    '-Wl,--gc-sections','-I',str(root/'include'),'-I',str(root/'userland/lib'),
                    str(f),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS: common-interval top CPU parser, idle accounting and task rows')
