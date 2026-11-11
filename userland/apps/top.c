/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A small top-like viewer using only existing /proc files. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "../lib/screen.h"

#define TOP_ROWS 14
#define TOP_COLS 79
#define TASK_COUNT 6

/* /proc files are finite, but read() may return partial data. */
static int read_proc(const char *path,char *out,unsigned cap){
    int fd=open(path,O_RDONLY);
    if(fd<0)return -1;
    unsigned len=0;
    int n=0;
    while(len<cap-1 && (n=read(fd,out+len,cap-1-len))>0)
        len+=(unsigned)n;
    close(fd);
    if(n<0)return -1;
    out[len]=0;
    return (int)len;
}

/* Convert one decimal value into tenths without floating-point code. */
static int tenths(const char **text){
    const char *p=*text;
    while(*p && (*p<'0'||*p>'9'))p++;
    if(!*p)return -1;
    unsigned whole=0;
    while(*p>='0'&&*p<='9'){
        whole=whole*10u+(unsigned)(*p-'0');p++;
        if(whole>10000u)return -1;
    }
    unsigned fraction=0;
    if(*p=='.'){
        p++;
        if(*p>='0'&&*p<='9')fraction=(unsigned)(*p++-'0');
        while(*p>='0'&&*p<='9')p++;
    }
    *text=p;
    return (int)(whole*10u+fraction);
}

/* Ignore the historical rows: only parse the newest finished one-second sample. */
static int cpu_latest(const char *input,unsigned values[5]){
    const char *p=input;
    for(int n=0;n<3;n++){
        p=strchr(p,'\n');
        if(!p)return -1;
        p++;
    }
    const char *end=strchr(p,'\n');
    if(!end)return -1;
    p=strchr(p,'|');
    if(!p||p>=end)return -1;
    p++;
    for(int i=0;i<5;i++){
        int v=tenths(&p);
        if(v<0||p>end)return -1;
        values[i]=(unsigned)v;
    }
    return 0;
}

/* Update a screen row only if its content changed. */
static void show_line(char previous[TOP_ROWS][TOP_COLS+1],int row,const char *text){
    char clipped[TOP_COLS+1];
    unsigned len=(unsigned)strlen(text);
    if(len>TOP_COLS)len=TOP_COLS;
    memcpy(clipped,text,len);clipped[len]=0;
    if(!strcmp(previous[row],clipped))return;
    screen_move(row+1,1);
    printf("\033[?7l%s\033[K\033[?7h",clipped);
    strcpy(previous[row],clipped);
}

/* Parse the kernel's task table, then format compact rows for 80-column TTYs. */
static int task_lines(char previous[TOP_ROWS][TOP_COLS+1],const char *data){
    const char *p=strchr(data,'\n');
    if(!p)return -1;
    p++;
    int count=0;
    char line[110],formatted[TOP_COLS+1];
    for(int row=0;row<TASK_COUNT;row++){
        if(*p){
            const char *end=strchr(p,'\n');
            if(!end)end=p+strlen(p);
            unsigned len=(unsigned)(end-p);
            if(len>=sizeof line)len=sizeof line-1;
            memcpy(line,p,len);line[len]=0;
            unsigned pid=0,slot=0,ppid=0;
            char name[19],state[10],tty[10],memory[20],sp[12];
            if(sscanf(line,"%u %18s %9s %9s %u %19s %11s %u",
                      &pid,name,state,tty,&slot,memory,sp,&ppid)==8){
                snprintf(formatted,sizeof formatted,
                         "%3u  %-14.14s  %-7.7s %-7.7s %4u  %-17.17s  %-8.8s %4u",
                         pid,name,state,tty,slot,memory,sp,ppid);
                count++;
            }else snprintf(formatted,sizeof formatted,"%s",line);
            p=*end?end+1:end;
        }else formatted[0]=0;
        show_line(previous,row+7,formatted);
    }
    return count;
}

/* Format a CPU percentage in fixed-width output. */
static void fmt_percent(char *out,unsigned n){
    snprintf(out,12,"%u.%u%%",n/10u,n%10u);
}

/* Refresh only changed terminal lines, without allocating a screen-sized buffer. */
static void refresh(char previous[TOP_ROWS][TOP_COLS+1],unsigned interval){
    char cpu[450],tasks[720],uptime[40],switches[40];
    char row[TOP_COLS+1];
    unsigned values[5];
    int cpu_ok=read_proc("/proc/cpu",cpu,sizeof cpu)>=0&&cpu_latest(cpu,values)==0;
    int tasks_ok=read_proc("/proc/tasks",tasks,sizeof tasks)>=0;
    int up_ok=read_proc("/proc/uptime",uptime,sizeof uptime)>=0;
    int sw_ok=read_proc("/proc/context_switches",switches,sizeof switches)>=0;
    unsigned up=up_ok?(unsigned)strtoul(uptime,0,10):0u;
    unsigned sw=sw_ok?(unsigned)strtoul(switches,0,10):0u;
    snprintf(row,sizeof row,"PicoNIX top     Uptime: %02u:%02u:%02u     Refresh: %us",
             up/3600u,(up/60u)%60u,up%60u,interval);
    show_line(previous,0,row);
    if(cpu_ok){
        char idle[12],busy[12],con[12],task[12],net[12];
        fmt_percent(idle,values[0]);fmt_percent(busy,values[1]);
        fmt_percent(con,values[2]);fmt_percent(task,values[3]);fmt_percent(net,values[4]);
        snprintf(row,sizeof row,"CPU: Idle %s  Busy %s  (Console %s  Task %s  Net %s)",
                 idle,busy,con,task,net);
    }else snprintf(row,sizeof row,"CPU: waiting for first one-second sample / unavailable");
    show_line(previous,1,row);
    if(tasks_ok){
        int count=0;
        for(const char *p=strchr(tasks,'\n');p&&*p;){
            p++;
            if(*p)count++;
            p=strchr(p,'\n');
        }
        snprintf(row,sizeof row,"Tasks: %d/%d     Context switches: %s",count,TASK_COUNT,sw_ok?"":"n/a");
        if(sw_ok)snprintf(row,sizeof row,"Tasks: %d/%d     Context switches: %u",count,TASK_COUNT,sw);
    }else snprintf(row,sizeof row,"Tasks: unavailable     Context switches: %s",sw_ok?"available":"n/a");
    show_line(previous,2,row);
    show_line(previous,3,"");
    show_line(previous,4,"PID  NAME            STATE   TTY     SLOT  MEMORY             SP       PPID");
    show_line(previous,5,"-------------------------------------------------------------------------------");
    show_line(previous,6,"");
    if(tasks_ok && task_lines(previous,tasks)>=0){}
    else for(int n=0;n<TASK_COUNT;n++)show_line(previous,7+n,n==0?"/proc/tasks: unavailable":"");
    show_line(previous,13,"q: Quit    Ctrl+C: Interrupt    CPU load is system-wide (approximate)");
    fflush(stdout);
}

/* Present live process and CPU summaries until quit. */
int main(int argc,char **argv){
    unsigned interval=1;
    if(argc>2){puts("usage: top [seconds] (1..60)");return 1;}
    if(argc==2){
        char *end;
        unsigned long n=strtoul(argv[1],&end,10);
        if(!argv[1][0]||*end||n<1||n>60){puts("usage: top [seconds] (1..60)");return 1;}
        interval=(unsigned)n;
    }
    int canon=ioctl(0,OS_TTY_GET_CANON,0);
    int echo=ioctl(0,OS_TTY_GET_ECHO,0);
    int flags=fcntl(0,F_GETFL,0);
    if(canon<0||echo<0||flags<0){puts("top: terminal required");return 1;}
    if(ioctl(0,OS_TTY_SET_CANON,0)<0||ioctl(0,OS_TTY_SET_ECHO,0)<0){
        ioctl(0,OS_TTY_SET_CANON,canon);ioctl(0,OS_TTY_SET_ECHO,echo);
        puts("top: terminal setup failed");return 1;
    }
    if(fcntl(0,F_SETFL,flags|O_NONBLOCK)<0){
        ioctl(0,OS_TTY_SET_ECHO,echo);ioctl(0,OS_TTY_SET_CANON,canon);
        puts("top: cannot enable nonblocking input");return 1;
    }
    char previous[TOP_ROWS][TOP_COLS+1]={0};
    screen_enter();
    int running=1;
    while(running){
        refresh(previous,interval);
        for(unsigned n=0;n<interval*20u&&running;n++){
            char c;
            int got=read(0,&c,1);
            if(got==1&&(c=='q'||c=='Q'||c==3))running=0;
            else if(got<0&&got!=OS_EAGAIN)running=0;
            if(running)msleep(50);
        }
    }
    screen_leave();
    fcntl(0,F_SETFL,flags);
    ioctl(0,OS_TTY_SET_ECHO,echo);
    ioctl(0,OS_TTY_SET_CANON,canon);
    puts("");
    return 0;
}
