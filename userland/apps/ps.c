/* SPDX-License-Identifier: GPL-3.0-or-later */
/* /proc/tasksの詳細情報を80桁以内のプロセス一覧として表示する． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

/* /proc/tasksはtop等とも共有するため，元の12フィールド形式を変更しない． */
static void show_task(const char *line) {
    unsigned pid,ppid,dispatch,ticks;
    char name[19],state[10],tty[10],slot[8],memory[20],sp[12];
    int fields=sscanf(line,"%u %18s %9s %9s %7s %19s %11s %u %u %u",
                      &pid,name,state,tty,slot,memory,sp,
                      &ppid,&dispatch,&ticks);
    if(fields!=10) return;
    /* 詳細なアドレスや累積Net/Console時間はcat /proc/tasksで確認できる． */
    printf("%3u  %-12.12s %-7.7s %-6.6s %4s %5u %10u %10u\n",
           pid,name,state,tty,slot,ppid,dispatch,ticks);
}

/* read()が行の途中で返っても，各タスクを1行ずつ整形する． */
int main(int argc,char **argv) {
    (void)argv;
    if(argc!=1) {puts("usage: ps");return 1;}
    int fd=open("/proc/tasks",O_RDONLY);
    if(fd<0) {puts("ps: cannot open /proc/tasks");return 1;}
    puts("PID  NAME         STATE   TTY    SLOT  PPID   DISPATCH  CPU_TICKS");
    char line[192],buf[64];
    unsigned used=0;
    int overflow=0,header=1,n;
    while((n=read(fd,buf,sizeof buf))>0) {
        for(int i=0;i<n;i++) {
            if(buf[i]=='\n') {
                if(!overflow) {
                    line[used]=0;
                    if(!header) show_task(line);
                }
                used=0;overflow=0;header=0;
            } else if(!overflow) {
                if(used+1<sizeof line) line[used++]=buf[i];
                else overflow=1;
            }
        }
    }
    if(n>=0 && used && !overflow && !header) {
        line[used]=0;
        show_task(line);
    }
    close(fd);
    return n<0 ? 1 : 0;
}
