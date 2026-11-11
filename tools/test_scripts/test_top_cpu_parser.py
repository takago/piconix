#!/usr/bin/env python3
"""Compile and run top's actual CPU parser against the /proc/cpu table contract."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
source=root/'userland/apps/top.c'
assert source.exists()
fixture='''       |                CPU Usage (%)                 |                  CPU Time (ms)
Time   |    Idle    Busy  ( Console    Task    Net )  |      Idle     Busy  ( Console     Task     Net )
-------+----------------------------------------------+-------------------------------------------------
   26s |    93.7     6.3  (     2.3     0.0     4.0)  |     936.9     63.1  (    22.8      0.0     40.3)
   25s |    93.9     6.1  (     2.1     0.0     3.9)  |     939.3     60.7  (    21.4      0.0     39.4)
'''
with tempfile.TemporaryDirectory() as tmp:
    f=Path(tmp)/'main.c'
    literal=''.join('\\n' if ch=='\n' else '\\"' if ch=='"' else ch for ch in fixture)
    f.write_text(f'''#define PICONIX_HOST_TEST 1\n#define main piconix_top_main
#include "{source}"
#undef main
int main(void){{
  const char *sample="{literal}";
  unsigned v[5]={{0}};
  if(cpu_latest(sample,v)!=0)return 1;
  if(v[0]!=937||v[1]!=63||v[2]!=23||v[3]!=0||v[4]!=40)return 2;
  if(cpu_latest("Time | Idle\\n-------\\n",v)==0)return 3;
  char prev[TOP_ROWS][TOP_COLS+1]={{0}};
  const char *tasks="PID  NAME STATE TTY SLOT MEMORY SP PPID\\n"
                    "  1  sh                BLOCKED  console  1     2000E000-20013FFF  20013ABC    0\\n"
                    " 12  top               READY    pts/0    4     20020000-20025FFF  20025ABC    1\\n";
  if(task_lines(prev,tasks)!=2)return 4;
  if(!strstr(prev[7],"sh")||!strstr(prev[8],"top"))return 5;
  if(task_lines(prev,"PID NAME\\n")!=0)return 6;
  if(prev[7][0]||prev[8][0])return 7;
  return 0;
}}
''')
    binary=Path(tmp)/'test'
    subprocess.run(['gcc','-std=c11','-O2','-ffunction-sections','-fdata-sections',
                    '-Wl,--gc-sections','-I',str(root/'include'),'-I',str(root/'userland/lib'),
                    str(f),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('PASS: actual top CPU and task parsers')
