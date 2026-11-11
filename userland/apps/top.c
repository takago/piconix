/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A small top-like viewer using existing /proc files. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "../lib/screen.h"

#define TOP_ROWS 16
#define TOP_COLS 79
#define TASK_COUNT 7

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

/* CPU ticks, network time and console time are sampled from the same /proc/tasks read. */
typedef struct { unsigned pid, ticks; } task_sample_t;
static task_sample_t previous_sample[TASK_COUNT];
static unsigned previous_time, previous_net_us, previous_con_us;
static int sample_valid;

/* Read the task rows first, then derive one coherent set of CPU percentages. */
static int task_lines(char previous[TOP_ROWS][TOP_COLS+1],const char *data,
                      unsigned now,unsigned values[5]){
    const char *p=strchr(data,'\n');
    if(!p)return -1;
    p++;
    typedef struct {
        unsigned pid, ppid, dispatch, ticks, delta;
        char name[19],state[10],tty[10];
    } task_row_t;
    task_row_t rows[TASK_COUNT]={{0}};
    task_sample_t next_sample[TASK_COUNT]={{0}};
    unsigned net_us=0,con_us=0;
    int count=0;
    char line[160],formatted[TOP_COLS+1];
    for(int i=0;i<TASK_COUNT && *p;i++){
        const char *end=strchr(p,'\n');
        if(!end)end=p+strlen(p);
        unsigned len=(unsigned)(end-p);
        if(len>=sizeof line)len=sizeof line-1;
        memcpy(line,p,len);line[len]=0;
        char memory[20],sp[12],slot[8];
        task_row_t *r=&rows[count];
        unsigned net=0,con=0;
        int fields=sscanf(line,"%u %18s %9s %9s %7s %19s %11s %u %u %u %u %u",
                          &r->pid,r->name,r->state,r->tty,slot,memory,sp,
                          &r->ppid,&r->dispatch,&r->ticks,&net,&con);
        if(fields>=10){
            if(r->pid==0 && fields==12){net_us=net;con_us=con;}
            next_sample[count].pid=r->pid;
            next_sample[count].ticks=r->ticks;
            if(sample_valid){
                for(int j=0;j<TASK_COUNT;j++)
                    if(previous_sample[j].pid==r->pid &&
                       (r->pid==0 || previous_sample[j].ticks!=0)){
                        r->delta=r->ticks-previous_sample[j].ticks;
                        break;
                    }
            }
            count++;
        }
        p=*end?end+1:end;
    }
    unsigned elapsed=now-previous_time;
    unsigned percentages[TASK_COUNT]={0};
    for(int j=0;j<5;j++)values[j]=0;
    if(sample_valid && elapsed && count){
        unsigned task_ticks=0;
        for(int i=0;i<count;i++)if(rows[i].pid!=0)task_ticks+=rows[i].delta;
        /* Kernel polling runs in idle context, so remove it from idle time. */
        unsigned net= (net_us-previous_net_us)/1000u;
        unsigned con= (con_us-previous_con_us)/1000u;
        unsigned idle_ticks=0;
        for(int i=0;i<count;i++)if(rows[i].pid==0)idle_ticks=rows[i].delta;
        if(net>idle_ticks)net=idle_ticks;
        if(con>idle_ticks-net)con=idle_ticks-net;
        /* Use the measured interval for both summary and task rows. */
        unsigned task_pct=(unsigned)((task_ticks*1000u)/elapsed);
        if(task_pct>1000u)task_pct=1000u;
        unsigned net_pct=(unsigned)((net*1000u)/elapsed);
        if(net_pct>1000u-task_pct)net_pct=1000u-task_pct;
        unsigned con_pct=(unsigned)((con*1000u)/elapsed);
        if(con_pct>1000u-task_pct-net_pct)con_pct=1000u-task_pct-net_pct;
        values[0]=1000u-task_pct-net_pct-con_pct;
        values[1]=1000u-values[0];
        values[2]=con_pct;values[3]=task_pct;values[4]=net_pct;
        for(int i=0;i<count;i++){
            if(rows[i].pid==0)percentages[i]=values[0];
            else {
                percentages[i]=(unsigned)((rows[i].delta*1000u)/elapsed);
                if(percentages[i]>1000u)percentages[i]=1000u;
            }
        }
    }
    for(int i=0;i<TASK_COUNT;i++){
        if(i<count){
            task_row_t *r=&rows[i];
            unsigned pct=percentages[i];
            snprintf(formatted,sizeof formatted,
                     "%3u  %-12.12s %-7.7s %-7.7s %3u.%u%% %10u %4u",
                     r->pid,r->name,r->state,r->tty,pct/10u,pct%10u,r->dispatch,r->ppid);
        }else formatted[0]=0;
        show_line(previous,i+8,formatted);
    }
    if(count){
        memcpy(previous_sample,next_sample,sizeof next_sample);
        previous_time=now;previous_net_us=net_us;previous_con_us=con_us;
        sample_valid=1;
    }
    return count;
}

/* Cumulative exceptions and UART IRQs, in /proc/interrupts order. */
typedef struct { unsigned systick, pendsv, svc, uart1; } irq_sample_t;

/* Parse one cumulative counter without an absolute-address pointer table. */
static int parse_irq_line(const char **input,const char *label,unsigned *count,int last){
    const char *p=*input;
    while(*label){
        if(*p++!=*label++)return 0;
    }
    if(*p<'0'||*p>'9')return 0;
    unsigned value=0;
    while(*p>='0'&&*p<='9')value=value*10u+(unsigned)(*p++-'0');
    if(*p!='\n'&&*p!=0)return 0;
    if(!last&&*p!='\n')return 0;
    *count=value;
    *input=*p?p+1:p;
    return 1;
}

static int parse_irqs(const char *data,irq_sample_t *v){
    return parse_irq_line(&data,"SysTick ",&v->systick,0) &&
           parse_irq_line(&data,"PendSV ",&v->pendsv,0) &&
           parse_irq_line(&data,"SVC ",&v->svc,0) &&
           parse_irq_line(&data,"uart1 ",&v->uart1,1);
}

/* Format a CPU percentage in fixed-width output. */
static void fmt_percent(char *out,unsigned n){
    snprintf(out,12,"%u.%u%%",n/10u,n%10u);
}

/* Refresh only changed terminal lines, without allocating a screen-sized buffer. */
static void refresh(char previous[TOP_ROWS][TOP_COLS+1],unsigned interval){
    char uptime[40],switches[40],tick_text[40],interrupts[96];
    static char tasks[1100];
    char row[TOP_COLS+1];
    unsigned values[5];
    int cpu_ok=0;
    int tasks_ok=read_proc("/proc/tasks",tasks,sizeof tasks)>=0;
    int up_ok=read_proc("/proc/uptime",uptime,sizeof uptime)>=0;
    int sw_ok=read_proc("/proc/context_switches",switches,sizeof switches)>=0;
    int tick_ok=read_proc("/proc/ticks",tick_text,sizeof tick_text)>=0;
    irq_sample_t irq={0};
    int irq_ok=read_proc("/proc/interrupts",interrupts,sizeof interrupts)>=0 &&
               parse_irqs(interrupts,&irq);
    unsigned now=tick_ok?(unsigned)strtoul(tick_text,0,10):0u;
    unsigned up=up_ok?(unsigned)strtoul(uptime,0,10):0u;
    unsigned sw=sw_ok?(unsigned)strtoul(switches,0,10):0u;
    snprintf(row,sizeof row,"PicoNIX top     Uptime: %02u:%02u:%02u     Refresh: %us",
             up/3600u,(up/60u)%60u,up%60u,interval);
    show_line(previous,0,row);
    int had_sample=sample_valid;
    if(tasks_ok && tick_ok && task_lines(previous,tasks,now,values)>=0)cpu_ok=had_sample;
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
        snprintf(row,sizeof row,"Tasks: %d/%d     Context switches: %s",count>0?count-1:0,TASK_COUNT-1,sw_ok?"":"n/a");
        if(sw_ok)snprintf(row,sizeof row,"Tasks: %d/%d     Context switches: %u",count>0?count-1:0,TASK_COUNT-1,sw);
    }else snprintf(row,sizeof row,"Tasks: unavailable     Context switches: %s",sw_ok?"available":"n/a");
    show_line(previous,2,row);
    if(irq_ok){
        snprintf(row,sizeof row,"IRQ: SysTick %u  PendSV %u  SVC %u  UART1 %u",
                 irq.systick,irq.pendsv,irq.svc,irq.uart1);
        show_line(previous,3,row);
    }else{
        show_line(previous,3,"IRQ: unavailable");
    }
    show_line(previous,4,"");
    show_line(previous,5,"PID  NAME         STATE   TTY      CPU%   DISPATCH PPID");
    show_line(previous,6,"-------------------------------------------------------------------------------");
    show_line(previous,7,"");
    if(!tasks_ok || !tick_ok)
        for(int n=0;n<TASK_COUNT;n++)show_line(previous,8+n,n==0?"/proc/tasks: unavailable":"");
    show_line(previous,15,"q: Quit    Ctrl+C: Interrupt    CPU%: 1ms samples; kernel polling subtracted (approx.)");
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
    static char previous[TOP_ROWS][TOP_COLS+1];
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
