/* SPDX-License-Identifier: GPL-3.0-or-later */
/* PicoNIX shell: freestanding, slot-linked at 0x20010000.
 * No libc, SDK, static mutable state, or kernel RAM data references.
 * Syscalls use SVC ABI: r0=number, r1..r3=arguments.
 */
#include <stdint.h>
#include <stddef.h>

enum { SYS_GETPID=1, SYS_KILL=2, SYS_OPEN=6, SYS_CLOSE=7,
       SYS_READ=8, SYS_WRITE=9, SYS_SPAWN=12, SYS_SLEEP=4, SYS_WAITPID=13, SYS_SET_FG=14, SYS_CONSOLE_READY=16 };
static inline int svc(int n, uintptr_t a, uintptr_t b, uintptr_t c) {
    register uintptr_t r0 __asm("r0") = (uintptr_t)n;
    register uintptr_t r1 __asm("r1") = a;
    register uintptr_t r2 __asm("r2") = b;
    register uintptr_t r3 __asm("r3") = c;
    __asm volatile("svc #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3) : "memory");
    return (int)r0;
}
static size_t slen(const char *s) { size_t n=0; while(s[n]) n++; return n; }
static int eq(const char *a,const char *b) {
    while(*a && *a==*b) { a++; b++; }
    return *a==*b;
}
static int prefix(const char *a,const char *b) {
    while(*b) if(*a++!=*b++) return 0;
    return 1;
}
/* A console write can be partial when the kernel TX queue fills.
 * Sleep to let the idle task drain it; never discard the remainder. */
static void write_all(int fd, const char *p, size_t n) {
    while(n) {
        int rc=svc(SYS_WRITE,fd,(uintptr_t)p,n);
        if(rc<0) return;
        if(rc==0) { svc(SYS_SLEEP,1,0,0); continue; }
        p+=(unsigned)rc; n-=(unsigned)rc;
    }
}
static void put(const char *s) {
    char b[48];
    while(*s) {
        size_t n=0;
        while(n<sizeof b && *s) b[n++]=*s++;
        write_all(1,b,n);
    }
}
static void number(int v) {
    unsigned x=(unsigned)v;
    if(v<0) { put("-"); x=0u-x; }
    const unsigned powers[10]={1000000000u,100000000u,10000000u,1000000u,100000u,10000u,1000u,100u,10u,1u};
    int emitted=0;
    char out[11]; unsigned n=0;
    for(unsigned k=0;k<10;k++) {
        unsigned div=powers[k];
        unsigned digit=0;
        while(x>=div) { x-=div; digit++; }
        if(digit || emitted || div==1u) {
            out[n++]=(char)('0'+digit); emitted=1;
        }
    }
    write_all(1,out,n);
}
static int parse_pid(const char *s) {
    if(!*s) return -1;
    int n=0;
    while(*s) {
        if(*s<'0'||*s>'9'||n>1000000) return -1;
        n=n*10+(*s++-'0');
    }
    return n>0?n:-1;
}
static void ps(void) {
    int fd=svc(SYS_OPEN,(uintptr_t)"/sys/proc/tasks",0,0);
    if(fd<0) { put("ps: cannot open tasks\r\n"); return; }
    char b[32]; int n;
    while((n=svc(SYS_READ,fd,(uintptr_t)b,sizeof b))>0)
        write_all(1,b,(size_t)n);
    if(n<0) put("ps: read error\r\n");
    svc(SYS_CLOSE,fd,0,0);
}
/* Shell-local cwd: the kernel still accepts absolute virtual paths only. */
static int directory(const char *p) {
    return eq(p,"/") || eq(p,"/dev") || eq(p,"/dev/gpio") ||
           eq(p,"/sys") || eq(p,"/sys/proc");
}
/* Resolve '.', '..', repeated '/' and relative names without libc. */
static int resolve(const char *cwd, const char *input, char out[128]) {
    char combined[256];
    unsigned n=0;
    if(*input!='/') {
        for(const char *q=cwd;*q;q++) { if(n>=sizeof(combined)-2) return -1; combined[n++]=*q; }
        if(n==0 || combined[n-1]!='/') combined[n++]='/';
    }
    for(const char *q=input;*q;q++) { if(n>=sizeof(combined)-1) return -1; combined[n++]=*q; }
    combined[n]=0;
    unsigned o=1; out[0]='/'; out[1]=0;
    for(unsigned i=0;i<n;) {
        while(combined[i]=='/') i++;
        if(!combined[i]) break;
        unsigned start=i;
        while(combined[i] && combined[i]!='/') i++;
        unsigned len=i-start;
        if(len==1 && combined[start]=='.') continue;
        if(len==2 && combined[start]=='.' && combined[start+1]=='.') {
            if(o>1) { while(o>1 && out[o-1]!='/') o--; if(o>1) o--; }
            out[o]=0;
            continue;
        }
        if(o>1) { if(o>=127) return -1; out[o++]='/'; }
        if(o+len>=128) return -1;
        for(unsigned k=0;k<len;k++) out[o++]=combined[start+k];
        out[o]=0;
    }
    return 0;
}
static void ls(const char *path) {
    if(eq(path,"/")) { put("dev/\r\nsys/\r\n"); return; }
    if(eq(path,"/dev") || eq(path,"/dev/")) {
        int fd=svc(SYS_OPEN,(uintptr_t)"/sys/proc/devices",0,0);
        if(fd<0) { put("ls: device list unavailable\r\n"); return; }
        char b[32]; int n;
        while((n=svc(SYS_READ,fd,(uintptr_t)b,sizeof b))>0)
            write_all(1,b,(size_t)n);
        svc(SYS_CLOSE,fd,0,0);
    }
    else if(eq(path,"/dev/gpio") || eq(path,"/dev/gpio/")) {
        for(int i=0;i<=28;i++) {
            number(i); put("  ");
            number(i); put(".dir  ");
            number(i); put(".pull\r\n");
        }
    } else if(eq(path,"/sys") || eq(path,"/sys/")) put("proc/\r\n");
    else if(eq(path,"/sys/proc") || eq(path,"/sys/proc/")) put("uptime  ticks  interrupts  context_switches  memory  devices  tasks\r\n");
    else put("ls: supported: /dev /dev/gpio /sys /sys/proc\r\n");
}
static void command(char *s, char cwd[128]) {
    while(*s==' ') s++;
    if(!*s) return;
    if(eq(s,"help")) {
        put("PicoNIX command help\r\n"
            "\r\nTask management:\r\n"
            "  help             Show this help\r\n"
            "  ps               Show tasks and RAM layout\r\n"
            "  pid              Show current process ID\r\n"
            "  blink [&]        Run blink (Ctrl+C to interrupt)\r\n"
            "  omikuji          Prompt for countdown seconds (foreground)\r\n"
            "  omikuji N [&]    Countdown 1-30 seconds; & = background\r\n"
            "  omikuji &        Background with default 10 seconds\r\n"
            "  &                Return to shell without waiting\r\n"
            "  Ctrl+C           Interrupt foreground task\r\n"
            "  kill PID         Terminate a task\r\n"
            "\r\nFiles and devices:\r\n"
            "  pwd              Print current directory\r\n"
            "  cd [PATH]        Change directory (default /)\r\n"
            "  ls [PATH]        List current or specified directory\r\n"
            "  ls /dev          List devices\r\n"
            "  ls /dev/gpio     List GPIO pins (N, N.dir, N.pull)\r\n"
            "  ls /sys/proc     List kernel statistics\r\n"
            "  cat PATH         Read file or device (UART1: 200ms idle exit)\r\n"
            "  Relative paths, . and .. are supported\r\n"
            "  echo TEXT        Print text\r\n"
            "  echo TEXT > PATH Write to a device\r\n"
            "\r\nExamples:\r\n"
            "  ps\r\n  blink &\r\n  omikuji\r\n  omikuji 5\r\n  omikuji 5 &\r\n  kill 2\r\n"
            "  cat /sys/proc/ticks\r\n"
            "  cat /sys/proc/interrupts\r\n"
            "  echo 1 > /dev/gpio/25\r\n"
            "  echo 0 > /dev/gpio/25\r\n"
            "  echo out > /dev/gpio/25.dir\r\n"
            "  cat /dev/gpio/25.dir\r\n"
            "  echo up > /dev/gpio/16.pull\r\n"
            "  cat /dev/gpio/16.pull\r\n");
    }
    else if(eq(s,"pwd")) { put(cwd); put("\r\n"); }
    else if(eq(s,"cd") || prefix(s,"cd ")) {
        const char *arg=eq(s,"cd")?"/":s+3;
        while(*arg==' ') arg++;
        if(!*arg) arg="/";
        char path[128];
        if(resolve(cwd,arg,path)<0 || !directory(path)) { put("cd: no such directory\r\n"); return; }
        unsigned i=0; while((cwd[i]=path[i])) i++;
    }
    else if(eq(s,"ps")) ps();
    else if(eq(s,"pid")) { number(svc(SYS_GETPID,0,0,0)); put("\r\n"); }
    else if(prefix(s,"kill ")) {
        int pid=parse_pid(s+5);
        if(pid<0 || pid==svc(SYS_GETPID,0,0,0)) put("usage: kill PID (not shell)\r\n");
        else put(svc(SYS_KILL,pid,0,0)==0?"terminated\r\n":"kill: no such task\r\n");
    } else if(prefix(s,"blink") || prefix(s,"omikuji")) {
        int omikuji=prefix(s,"omikuji");
        const char *tail=s+(omikuji?7:5);
        while(*tail==' ') tail++;
        int seconds=0, background=0;
        if(omikuji && *tail>='0' && *tail<='9') {
            while(*tail>='0' && *tail<='9') {
                seconds=seconds*10+(*tail++-'0');
                if(seconds>30) break;
            }
            while(*tail==' ') tail++;
            if(seconds<1 || seconds>30) { put("usage: omikuji [1-30] [&]\r\n"); return; }
        }
        if(*tail=='&') { background=1; tail++; while(*tail==' ') tail++; }
        if(*tail) { put(omikuji?"usage: omikuji [1-30] [&]\r\n":"usage: blink [&]\r\n"); return; }
        int pid=svc(SYS_SPAWN,omikuji?3:1,(uintptr_t)(omikuji && background && !seconds ? 31 : seconds),0);
        if(pid<0) { put("cannot start program (slot busy or invalid argument)\r\n"); return; }
        if(background) { put("["); number(pid); put(omikuji?"] omikuji started\r\n":"] blink started\r\n"); }
        else {
            int status=0;
            svc(SYS_SET_FG,pid,0,0);
            for(;;) {
                int rc=svc(SYS_WAITPID,pid,(uintptr_t)&status,0);
                if(rc==pid) break;
                if(rc<0) { put("waitpid: error\r\n"); break; }
                svc(SYS_SLEEP,10,0,0);
            }
            svc(SYS_SET_FG,0,0,0);
        }
    } else if(eq(s,"ls") || prefix(s,"ls ")) {
        const char *arg=eq(s,"ls")?cwd:s+3;
        while(*arg==' ') arg++;
        if(!*arg) arg=cwd;
        char path[128];
        if(resolve(cwd,arg,path)<0) put("ls: path too long\r\n");
        else ls(path);
    }
    else if(prefix(s,"cat ")) {
        char path[128];
        if(resolve(cwd,s+4,path)<0) { put("cat: path too long\r\n"); return; }
        if(eq(path,"/dev/console")) { put("cat: console reserved for shell\r\n"); return; }
        int uart=eq(path,"/dev/uart1");
        int fd=svc(SYS_OPEN,(uintptr_t)path,uart?4:0,0);
        if(fd<0) { put("cat: cannot open\r\n"); return; }
        // Proc reports can exceed one read. Read until EOF, using small
        // transfers to avoid overflowing the nonblocking USB console FIFO.
        if(prefix(path,"/sys/proc/")) {
            char b[32]; int n;
            while((n=svc(SYS_READ,fd,(uintptr_t)b,sizeof b))>0) {
                write_all(1,b,(size_t)n);
            }
            if(n<0) put("cat: read error\r\n");
            else put("\r\n");
        } else if(uart) {
            /* UART is a byte stream without EOF. Poll in 10ms intervals,
               stop after 200ms of inactivity, including before first byte. */
            char b[32]; unsigned idle=0;
            while(idle<200) {
                int n=svc(SYS_READ,fd,(uintptr_t)b,sizeof b);
                if(n<0) { put("cat: read error\r\n"); break; }
                if(n>0) { write_all(1,b,(size_t)n); idle=0; }
                else { svc(SYS_SLEEP,10,0,0); idle+=10; }
            }
            put("\r\n");
        } else {
            char b[32]; int n=svc(SYS_READ,fd,(uintptr_t)b,sizeof b);
            if(n>0) { write_all(1,b,(size_t)n); put("\r\n"); }
            else if(n<0) put("cat: read error\r\n");
        }
        svc(SYS_CLOSE,fd,0,0);
    } else if(prefix(s,"echo ")) {
        char *v=s+5,*redir=0;
        for(char *p=v;*p;p++) if(p[0]==' '&&p[1]=='>'&&p[2]==' ') { redir=p; break; }
        if(!redir) { put(v); put("\r\n"); return; }
        *redir=0;
        char path[128];
        if(resolve(cwd,redir+3,path)<0) { put("echo: path too long\r\n"); return; }
        int fd=svc(SYS_OPEN,(uintptr_t)path,1,0);
        if(fd<0) { put("echo: cannot open\r\n"); return; }
        if(svc(SYS_WRITE,fd,(uintptr_t)v,slen(v))<0) put("echo: write error\r\n");
        svc(SYS_CLOSE,fd,0,0);
    } else put("unknown command (help)\r\n");
}
__attribute__((used,noreturn,section(".text.app_entry")))
void app_entry(void) {
    char line[128];
    char cwd[128];
    cwd[0]='/';
    cwd[1]=0;
    /* USB CDC may enumerate after the shell task starts.  Do not discard the banner. */
    while(!svc(SYS_CONSOLE_READY,0,0,0)) svc(SYS_SLEEP,10,0,0);
    put("\r\n--------------------------------------------------------\r\n"
        "PicoNIX-2026-10-26 (TAKAGO_LAB.)\r\n"
        "Type 'help' for commands.\r\n"
        "--------------------------------------------------------\r\n"
        "piconix# ");
    for(;;) {
        int n=svc(SYS_READ,0,(uintptr_t)line,sizeof line-1);
        if(n<=0) continue;
        line[n]=0;
        while(n>0 && (line[n-1]=='\r'||line[n-1]=='\n')) line[--n]=0;
        /* Reap completed background children (no status display yet). */
        int bg_status;
        while(svc(SYS_WAITPID,(uintptr_t)-1,(uintptr_t)&bg_status,0)>0) {}
        command(line,cwd);
        put("piconix# ");
    }
}
