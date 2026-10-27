/* User-space PicoNIX kernel trace reader. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "trace.h"

static const char *syscall_name(unsigned n) {
    switch(n) {
        case 0: return "?";
        case 1: return "getpid";
        case 2: return "kill";
        case 3: return "exit";
        case 4: return "sleep";
        case 5: return "yield";
        case 6: return "open";
        case 7: return "close";
        case 8: return "read";
        case 9: return "write";
        case 10: return "ioctl";
        case 11: return "reserved";
        case 12: return "spawn";
        case 13: return "waitpid";
        case 14: return "set_fg";
        case 15: return "reserved";
        case 16: return "console_ready";
        case 17: return "unlink";
        case 18: return "dup2";
        case 19: return "tty_spawn";
        case 20: return "get_tty";
        case 21: return "lseek";
        default: return "unknown";
    }
}
int main(int argc, char *argv[])
{
    if(argc!=2 || (strcmp(argv[1],"on") && strcmp(argv[1],"off") && strcmp(argv[1],"show"))) {
        puts("usage: trace on|off|show"); return 1;
    }
    int fd=open("/dev/trace",O_RDONLY);
    if(fd<0) { puts("trace: cannot open /dev/trace"); return 1; }
    int show=!strcmp(argv[1],"show");
    int cmd=!strcmp(argv[1],"on") ? TRACE_ON : TRACE_OFF;
    if(ioctl(fd,show ? TRACE_OFF : cmd,0)<0) {
        puts("trace: ioctl failed"); close(fd); return 1;
    }
    if(!show) {
        puts(cmd==TRACE_ON ? "trace enabled (buffer cleared)" : "trace disabled");
        close(fd); return 0;
    }
    trace_event_t event;
    int count=0, n;
    while((n=read(fd,&event,sizeof event))==(int)sizeof event) {
        if(event.kind==TRACE_SWITCH)
            printf("[%u] SWITCH %u -> %u\n", event.tick, event.pid, event.value);
        else if(event.kind==TRACE_SVC_ENTER)
            printf("[%u] ENTER pid=%u %s(%u)\n",event.tick,event.pid,
                   syscall_name(event.value), event.detail);
        else if(event.kind==TRACE_SVC_EXIT)
            printf("[%u] EXIT  pid=%u %s => %d\n",event.tick,event.pid,
                   syscall_name(event.value),(int)event.detail);
        count++;
    }
    close(fd);
    if(n<0) { puts("trace: read error"); return 1; }
    if(!count) puts("trace: no events (use trace on first)");
    return 0;
}
