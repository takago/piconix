#!/usr/bin/env python3
"""Host-run the actual /proc/cpu table reader for 30s history and split reads."""
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
src=(root/'kernel/kernel.c').read_text()
start=src.index('NO_TRACE static int proc_cpu_read(')
stop=src.index('/* /procの読み取りを担当する．', start)
fn=src[start:stop].replace('NO_TRACE static int proc_cpu_read(', 'static int proc_cpu_read(', 1)
shim=r'''#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define CPU_HISTORY 30
#define OS_EINVAL (-22)
typedef struct {uint32_t time_ms,task_us,net_us,console_us;} cpu_record_t;
static volatile cpu_record_t cpu_history[CPU_HISTORY];
static volatile uint32_t cpu_history_serial;
typedef struct {int fd_task_pid[4];uint32_t fd_pos[4];} task_t;
static task_t tasks[1];
static int current;
static uint32_t save_and_disable_interrupts(void) {return 0;}
static void restore_interrupts(uint32_t x) {(void)x;}
'''
main=r'''int main(void) {
    for(unsigned i=0;i<30;i++){
      cpu_history[i]=(cpu_record_t){(i+1)*1000,100000,50000,25000};
    }
    cpu_history_serial=30;
    tasks[0].fd_task_pid[1]=30;
    char all[8192]={0};size_t pos=0;
    while(1) {
       int n=proc_cpu_read(1,all+pos,7);
       if(n<0)return 1;
       if(n==0)break;
       pos+=(size_t)n;
       if(pos>=sizeof all-16)return 2;
    }
    all[pos]=0;
    if(strstr(all,"CPU Usage (%)")==NULL || strstr(all,"CPU Time (ms)")==NULL)return 3;
    /* All header and sample lines have identical widths and separators. */
    char *line=all; unsigned rownum=0;
    while(*line) {
      char *end=strchr(line,'\n'); if(!end) return 11;
      size_t width=(size_t)(end-line);
      if(width!=104 || line[7] != (rownum==2?'+':'|') ||
         line[54] != (rownum==2?'+':'|')) return 12;
      if(rownum==1 && (line[26]!='(' || line[51]!=')' ||
                       line[76]!='(' || line[103]!=')'))return 13;
      if(rownum>2 && (line[26]!='(' || line[51]!=')' ||
                      line[76]!='(' || line[103]!=')'))return 14;
      line=end+1;rownum++;
    }

    if(strstr(all,"30s |") == NULL || strstr(all," 1s |") == NULL)return 4;
    if(strstr(all,"31s |"))return 5;
    if(strstr(all,"  82.5") == NULL || strstr(all," 17.5") == NULL || strstr(all,"825.0")==NULL)return 6;
    int rows=0;
    for(char *q=all;*q;q++) if(q[0]=='s' && q[1]==' ' && q[2]=='|')rows++;
    if(rows!=30) {fprintf(stderr,"rows=%d\n%s",rows,all);return 7;}
    tasks[0].fd_pos[1]=0;
    tasks[0].fd_task_pid[1]=0;
    int len=proc_cpu_read(1,all,sizeof all);
    if(len<=0 || strstr(all,"CPU Usage (%)")==NULL) return 8;
    /* After wrap, only the newest 30 samples should be visible. */
    for(unsigned number=10;number<40;number++){
      unsigned idx=number%30;
      cpu_history[idx]=(cpu_record_t){(number+1)*1000,100000,50000,25000};
    }
    cpu_history_serial=40;
    tasks[0].fd_task_pid[1]=40;
    tasks[0].fd_pos[1]=0;
    memset(all,0,sizeof all);
    len=proc_cpu_read(1,all,sizeof all-1);
    if(len<=0)return 9;
    all[len]=0;
    if(strstr(all,"40s |") == NULL || strstr(all,"11s |") == NULL || strstr(all,"10s |") != NULL)return 10;
    return 0;
}
'''

with tempfile.TemporaryDirectory() as d:
    c=Path(d)/'test.c';exe=Path(d)/'test'
    c.write_text(shim+fn+main)
    subprocess.run(['cc','-std=c11','-Wall','-Wextra',str(c),'-o',str(exe)],check=True,capture_output=True)
    subprocess.run([str(exe)],check=True,capture_output=True)
print('PASS: /proc/cpu 30s table, newest-first, percentages/ms, split reads and wrap')
