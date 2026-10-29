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
        case 22: return "chdir";
        case 23: return "getcwd";
        case 24: return "fcntl";
        default: return "unknown";
    }
}
/* /bin lists registered applications in program-ID order.  Resolve names
 * once at display time; the ID itself was recorded before the task exited. */
#define PROGRAM_COUNT 32
#define NAMES_CAP 768
static char names[NAMES_CAP];
static const char *program_names[PROGRAM_COUNT];

static void load_program_names(void) {
    for(int i=0;i<PROGRAM_COUNT;i++) program_names[i]="?";
    program_names[0]="idle";
    int fd=open("/bin",O_RDONLY);
    if(fd<0) return;
    unsigned pos=0;
    int n;
    while(pos<NAMES_CAP-1 && (n=read(fd,names+pos,NAMES_CAP-1-pos))>0)
        pos+=(unsigned)n;
    close(fd);
    names[pos]=0;
    int id=1;
    unsigned start=0;
    for(unsigned i=0;i<pos && id<PROGRAM_COUNT;i++) {
        if(names[i]=='\n') {
            names[i]=0;
            program_names[id++]=names+start;
            start=i+1;
        }
    }
}

static const char *program_name(unsigned id) {
    return id<PROGRAM_COUNT ? program_names[id] : "?";
}

/* Keep a small history of successfully opened descriptors. */
#define FD_NAMES 24
static struct { unsigned pid; unsigned fd; char name[17]; } fd_names[FD_NAMES];
static unsigned fd_next;
static const char *find_fd(unsigned pid, unsigned fd) {
    for(unsigned i=0;i<FD_NAMES;i++)
        if(fd_names[i].name[0] && fd_names[i].pid==pid && fd_names[i].fd==fd)
            return fd_names[i].name;
    return NULL;
}
static void forget_fd(unsigned pid, unsigned fd) {
    for(unsigned i=0;i<FD_NAMES;i++)
        if(fd_names[i].name[0] && fd_names[i].pid==pid && fd_names[i].fd==fd)
            fd_names[i].name[0]=0;
}
static void save_fd(unsigned pid, unsigned fd, const uint8_t *name) {
    forget_fd(pid,fd);
    unsigned i=fd_next++;
    if(fd_next==FD_NAMES) fd_next=0;
    fd_names[i].pid=pid; fd_names[i].fd=fd;
    for(unsigned j=0;j<16;j++) fd_names[i].name[j]=(char)name[j];
    fd_names[i].name[16]=0;
}
/* Data is a binary preview, not a NUL-terminated C string. */
/* Fixed columns without printf width modifiers (not supported by PicoNIX libc). */
static void col(const char *s, unsigned width) {
    unsigned i=0;
    while(i<width && s[i]) {putchar(s[i]); i++;}
    while(i++<width) putchar(' ');
    putchar(' ');
}
static void numeric(char *s, int n) {sprintf(s,"%d",n);}
static void format_short_time(char *out, uint32_t ms) {
    char digits[16];
    sprintf(digits,"%u",(unsigned)ms);
    unsigned len=(unsigned)strlen(digits), pos=0;
    if(len<=3) {
        out[pos++]='0'; out[pos++]='.';
        for(unsigned i=len;i<3;i++) out[pos++]='0';
        for(unsigned i=0;i<len;i++) out[pos++]=digits[i];
    } else {
        for(unsigned i=0;i<len-3;i++) out[pos++]=digits[i];
        out[pos++]='.';
        for(unsigned i=len-3;i<len;i++) out[pos++]=digits[i];
    }
    out[pos]=0;
}
static void append(char *out, const char *s) {
    unsigned n=(unsigned)strlen(out);
    while(*s && n<sizeof(((trace_event_t *)0)->data)*4u-1u) out[n++]=*s++;
    out[n]=0;
}
static void format_target(char *out, unsigned pid, unsigned fd, int8_t id) {
    const char *name=find_fd(pid,fd);
    sprintf(out,"%u",fd);
    if(name) {append(out,":"); append(out,name);}
    else if(id==0) append(out,":/dev/console");
    else if(id==30) append(out,":/dev/uart1");
    else if(id>=0) {char part[16];sprintf(part,":id=%d",(int)id);append(out,part);}
}
static void print_preview(const uint8_t *data, unsigned len, int shortened) {
    unsigned written=0;
    putchar('"'); written++;
    unsigned i=0;
    for(;i<len && written<9;i++) {
        unsigned c=data[i];
        if(c=='\n' || c=='\r' || c=='\t') {
            putchar('\\');putchar(c=='\n'?'n':c=='\r'?'r':'t'); written+=2;
        } else if(c=='"'||c=='\\') {putchar('\\');putchar((int)c);written+=2;}
        else if(c>=32 && c<127) {putchar((int)c);written++;}
        else {putchar('.');written++;}
    }
    putchar('"');
    if(shortened || i<len) printf("...");
}
static void header(void) {
    puts("Time   PID Command TTY     Event    Call     Target           Ret Data");
    puts("--------------------------------------------------------------------------");
}
static unsigned close_pid, close_fd;
static struct { unsigned pid, call; char target[64]; } pending[8];
static unsigned pending_next;
static const char *pending_find(unsigned pid, unsigned call) {
    for(unsigned i=0;i<8;i++)
        if(pending[i].pid==pid && pending[i].call==call)
            return pending[i].target;
    return NULL;
}
static void pending_save(unsigned pid, unsigned call, const char *target) {
    unsigned i=pending_next++ & 7u;
    pending[i].pid=pid; pending[i].call=call;
    strcpy(pending[i].target,target);
}
static void print_event(const trace_event_t *e) {
    char time[20],pid[16],target[64]="",ret[16]="";
    unsigned shown_pid=e->kind==TRACE_SWITCH ? e->value : e->pid;
    format_short_time(time,e->tick);
    numeric(pid,(int)shown_pid);
    const char *tty=shown_pid==0 ? "-" : (e->program_id&0x80u ? "uart1" : "console");
    col(time,7);col(pid,3);col(program_name(e->program_id&0x7fu),7);col(tty,7);
    if(e->kind==TRACE_SWITCH) {
        col("SWITCH",8);col("",8);
        sprintf(target,"%u -> %u",e->pid,e->value);
        col(target,16);putchar('\n');return;
    }
    if(e->kind==TRACE_TICK1K) {
        col("TICKx1K",8);col("",8);col("SysTick",16);putchar('\n');return;
    }
    if(e->kind!=TRACE_SVC_ENTER && e->kind!=TRACE_SVC_EXIT) {puts("UNKNOWN");return;}
    int enter=e->kind==TRACE_SVC_ENTER;
    unsigned call=e->value;
    if(!enter) {
        numeric(ret,(int)e->detail);
        if(call==6 && (int)e->detail>=0) {
            save_fd(e->pid,e->detail,e->data);
            format_target(target,e->pid,e->detail,e->file_id);
        } else {
            const char *prior=pending_find(e->pid,call);
            if(prior) strcpy(target,prior);
        }
    } else {
        if(call==7 || call==8 || call==9 || call==10 || call==21)
            format_target(target,e->pid,e->detail,e->file_id);
        else if(call==6) strcpy(target,"path");
        else if(call==13) numeric(target,(int32_t)e->detail);
        else if(call==12) {const char *name=program_name(e->detail);strcpy(target,name);}
        else numeric(target,(int32_t)e->detail);
        pending_save(e->pid,call,target);
        if(call==7) {close_pid=e->pid;close_fd=e->detail;}
    }
    col(enter?"ENTER":"EXIT",8);col(syscall_name(call),8);
    col(target,16);col(ret,3);
    if(!enter && call==6 && (int)e->detail>=0) {
        /* The open path is already available from the binary event. */
    } else if(!enter && (call==8 || call==9) && (int)e->detail>0 && e->file_id!=99) {
        unsigned length=(unsigned)e->detail;
        if(length>16) length=16;
        print_preview(e->data,length,(unsigned)e->detail>16);
    }
    putchar('\n');
    if(!enter && call==7 && (int)e->detail==0 && close_pid==e->pid)
        forget_fd(close_pid,close_fd);
}
int main(int argc, char *argv[]) {
    if(argc<2 || argc>3 || (strcmp(argv[1],"on") && strcmp(argv[1],"off") &&
                   strcmp(argv[1],"show") && strcmp(argv[1],"follow"))) {
        puts("usage: trace on|off|show [PID]|follow [PID]"); return 1;
    }
    int filter_pid=-1;
    if(argc==3) {
        if(strcmp(argv[1],"show") && strcmp(argv[1],"follow")) {
            puts("trace: PID filter requires show or follow"); return 1;
        }
        const char *p=argv[2];
        if(!*p) {puts("trace: invalid PID");return 1;}
        int pid=0;
        while(*p) {
            if(*p<'0' || *p>'9' || pid>100000) {
                puts("trace: invalid PID");return 1;
            }
            pid=pid*10+(*p++-'0');
        }
        filter_pid=pid;
    }
    int fd=open("/dev/trace",O_RDONLY);
    if(fd<0) {puts("trace: cannot open /dev/trace"); return 1;}
    int show=!strcmp(argv[1],"show"), follow=!strcmp(argv[1],"follow");
    if(!show && !follow) {
        int on=!strcmp(argv[1],"on");
        if(ioctl(fd,on?TRACE_ON:TRACE_OFF,0)<0) {puts("trace: ioctl failed");close(fd);return 1;}
        puts(on?"trace enabled (buffer cleared)":"trace disabled");
        close(fd);return 0;
    }
    if(show && ioctl(fd,TRACE_OFF,0)<0) {puts("trace: ioctl failed");close(fd);return 1;}
    load_program_names();
    header();
    trace_event_t event;
    int count=0,n;
    for(;;) {
        if(follow) {
            int lost=ioctl(fd,TRACE_LOST,0);
            if(lost>0) printf("trace: %d events overwritten\n",lost);
        }
        n=read(fd,&event,sizeof event);
        if(n==(int)sizeof event) {
            /* Switch events identify the destination task in value. */
            unsigned event_pid=event.kind==TRACE_SWITCH ? event.value : event.pid;
            if(filter_pid<0 || event_pid==(unsigned)filter_pid) {
                print_event(&event);count++;
            }
            continue;
        }
        if(n<0) {puts("trace: read error");close(fd);return 1;}
        if(!follow) break;
        if(ioctl(fd,TRACE_STATUS,0)==0) break;
        msleep(50);
    }
    close(fd);
    if(!count) puts("trace: no events (use trace on first)");
    return 0;
}
