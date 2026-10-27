/* SPDX-License-Identifier: GPL-3.0-or-later */
/* PicoNIX shell: freestanding, slot-linked at 0x20018000.
 * Small user libc; no SDK or kernel RAM data references.
 * Syscalls use SVC ABI: r0=number, r1..r3=arguments.
 */
#include <stdint.h>
#include <stddef.h>

#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "stdio.h"
static int eq(const char *a,const char *b) { return strcmp(a,b)==0; }
static int prefix(const char *a,const char *b) { return strncmp(a,b,strlen(b))==0; }
/* A console write can be partial when the kernel TX queue fills.
 * Sleep to let the idle task drain it; never discard the remainder. */
static void write_all(int fd, const char *p, size_t n) {
    while(n) {
        int rc=write(fd, p, n);
        if(rc<0) return;
        if(rc==0) { sleep(1); continue; }
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
    int fd=open("/proc/tasks", 0);
    if(fd<0) { put("ps: cannot open tasks\r\n"); return; }
    char b[32]; int n;
    while((n=read(fd, b, sizeof b))>0)
        write_all(1,b,(size_t)n);
    if(n<0) put("ps: read error\r\n");
    close(fd);
}
/* Shell-local cwd: the kernel still accepts absolute virtual paths only. */
static int directory(const char *p) {
    return eq(p,"/") || eq(p,"/dev") || eq(p,"/dev/gpio") ||
           eq(p,"/proc") || eq(p,"/tmp");
}
/* Resolve '.', '..', repeated '/' and relative names with a small user libc. */
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
    if(eq(path,"/")) { put("dev/\r\nproc/\r\ntmp/\r\n"); return; }
    if(eq(path,"/dev") || eq(path,"/dev/") ||
       eq(path,"/tmp") || eq(path,"/tmp/")) {
        const char *listing=eq(path,"/dev") || eq(path,"/dev/") ?
            "/proc/devices" : "/tmp";
        int fd=open(listing, 0);
        if(fd<0) { put("ls: cannot open directory\r\n"); return; }
        char b[32]; int n;
        while((n=read(fd, b, sizeof b))>0)
            write_all(1,b,(size_t)n);
        if(n<0) put("ls: read error\r\n");
        close(fd);
    }
    else if(eq(path,"/dev/gpio") || eq(path,"/dev/gpio/")) {
        for(int i=0;i<=28;i++) {
            number(i); put("\r\n");
            number(i); put(".dir\r\n");
            number(i); put(".pull\r\n");
        }
    } else if(eq(path,"/proc") || eq(path,"/proc/"))
        put("uptime\r\nticks\r\ninterrupts\r\ncontext_switches\r\nmemory\r\ndevices\r\ntasks\r\n");
    else put("ls: supported: /dev /dev/gpio /proc /tmp\r\n");
}
/* Whitespace is syntax around redirection paths, not part of names. */
static char *trim(char *s) {
    while(*s==' ' || *s=='\t') s++;
    char *end=s+strlen(s);
    while(end>s && (end[-1]==' ' || end[-1]=='\t')) *--end=0;
    return s;
}
/* Redirection applies to builtins and spawned processes alike.  Return the
 * temporary descriptor so that the caller can restore the standard stream. */
static int redirect(char *s, const char *cwd, int *save_in, int *save_out) {
    char *first=0, *last=0;
    for(char *q=s;*q;q++) if(*q=='<' || *q=='>') {
        if(!first) first=q;
        last=q;
        if(*q=='>' && q[1]=='>') q++;
    }
    if(!first) return 0;
    /* A file name must be one unquoted word.  Two redirects are allowed. */
    char *q=first;
    int seen=0;
    while(*q) {
        if(*q!='<' && *q!='>') return -1;
        int output=*q=='>';
        q++;
        int append=output && *q=='>';
        if(append) q++;
        while(*q==' ' || *q=='\t') q++;
        char *name=q;
        while(*q && *q!=' ' && *q!='\t' && *q!='<' && *q!='>') q++;
        if(name==q) return -1;
        char terminator=*q;
        *q=0;
        char path[128];
        int valid=resolve(cwd,name,path);
        *q=terminator;
        if(valid<0) return -1;
        /* Device nodes are not regular files: creation/truncation is invalid. */
        int flag=output?(1|(prefix(path,"/dev/")?0:O_CREAT|(append?O_APPEND:O_TRUNC))):0;
        int target=output?1:0, saved=output?7:6;
        if(seen & (1<<target)) return -1;
        seen|=1<<target;
        int fd=open(path, flag);
        if(fd<0) return -1;
        if(fd==saved || dup2(target, saved)<0 ||
           dup2(fd, target)<0) {
            close(fd); return -1;
        }
        close(fd);
        if(output) *save_out=saved; else *save_in=saved;
        while(*q==' ' || *q=='\t') q++;
        if(*q && *q!='<' && *q!='>') return -1;
    }
    *first=0;
    return 0;
}
static void command(char *s, char cwd[128], int redirect_out, int requested_bg, int *bg_pid, const char **bg_name) {
    while(*s==' ') s++;
    if(!*s) return;
    /* Background execution is meaningful only for spawned programs. */
    if(requested_bg && !(eq(s,"blink") || prefix(s,"blink ") ||
                         eq(s,"omikuji") || prefix(s,"omikuji ") ||
                         eq(s,"python") || prefix(s,"python ") ||
                         eq(s,"hello") || prefix(s,"hello ") ||
                         eq(s,"hexdump") || prefix(s,"hexdump ") ||
                         eq(s,"trace") || prefix(s,"trace ") || prefix(s,"getty "))) {
        put("background: requires an external program\r\n");
        return;
    }
    if(eq(s,"help")) {
        put("PicoNIX command help\r\n"
            "\r\nTask management:\r\n"
            "  help             Show this help\r\n"
            "  ps               Show tasks and RAM layout\r\n"
            "  pid              Show current process ID\r\n"
            "  tty              Show controlling terminal\r\n"
            "  logout / exit    Close this shell session\r\n"
            "  blink [&]        Run blink (Ctrl+C to interrupt)\r\n"
            "  omikuji          Prompt for countdown seconds (foreground)\r\n"
            "  omikuji N [&]    Countdown 1-30 seconds; & = background\r\n"
            "  omikuji &        Background with default 10 seconds\r\n"
            "  python [file] [&] REPL or run a /tmp script\r\n"
            "  hello [name] [&] Say hello\r\n"
            "  hexdump [file [offset [count]]] [&]  Hex/ASCII dump\r\n"
            "  trace on|off|show   Record or display kernel events\r\n"
            "  getty /dev/uart1 sh|python [&]  Attach a terminal\r\n"
            "  &                Return to shell without waiting\r\n"
            "  Ctrl+C           Interrupt foreground task\r\n"
            "  kill PID         Terminate a task\r\n"
            "\r\nFiles and devices:\r\n"
            "  pwd              Print current directory\r\n"
            "  cd [PATH]        Change directory (default /)\r\n"
            "  ls [PATH]        List current or specified directory\r\n"
            "  ls /dev          List devices\r\n"
            "  ls /dev/gpio     List GPIO pins (N, N.dir, N.pull)\r\n"
            "  ls /proc     List kernel statistics\r\n"
            "  cat PATH         Read file or device (UART1: 200ms idle exit)\r\n"
            "  cat > PATH       Enter file contents; Ctrl+D ends input\r\n"
            "  rm PATH          Delete /tmp file\r\n"
            "  Relative paths, . and .. are supported\r\n"
            "  echo TEXT        Print text\r\n"
            "  echo TEXT > PATH Write or create file/device\r\n"
            "  COMMAND < PATH  Redirect input\r\n"
            "  COMMAND > PATH  Redirect output\r\n"
            "  COMMAND >> PATH Append output\r\n"
            "  omikuji N > PATH &  Redirect background output\r\n"
            "\r\nExamples:\r\n"
            "  ps\r\n  blink &\r\n  omikuji\r\n  omikuji 5\r\n  omikuji 5 &\r\n  kill 2\r\n"
            "  cat /proc/ticks\r\n"
            "  cat /proc/interrupts\r\n"
            "  echo 1 > /dev/gpio/25\r\n"
            "  echo 0 > /dev/gpio/25\r\n"
            "  echo out > /dev/gpio/25.dir\r\n"
            "  cat /dev/gpio/25.dir\r\n"
            "  echo up > /dev/gpio/16.pull\r\n"
            "  cat /dev/gpio/16.pull\r\n"
            "  hexdump /dev/mem 0x20018000 64\r\n"
            "  hexdump /dev/mem 0x10000000 64\r\n"
            "  trace on\r\n  hello\r\n  trace off\r\n  trace show\r\n");
    }
    else if(eq(s,"logout") || eq(s,"exit")) {
        put("logout\r\nPicoNIX: Session closed.\r\n");
        _exit(0);
        for(;;) sleep(100);
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
    else if(eq(s,"pid")) { number(getpid()); put("\r\n"); }
    else if(eq(s,"tty")) {
        int tty=get_tty();
        put(tty==0?"/dev/console\r\n":tty==1?"/dev/uart1\r\n":"not a tty\r\n");
    }
    else if(prefix(s,"kill ")) {
        int pid=parse_pid(s+5);
        if(pid<0 || pid==getpid()) put("usage: kill PID (not shell)\r\n");
        else put(kill(pid)==0?"terminated\r\n":"kill: no such task\r\n");
    } else if(prefix(s,"blink") || prefix(s,"omikuji")) {
        int omikuji=prefix(s,"omikuji");
        const char *tail=s+(omikuji?7:5);
        while(*tail==' ') tail++;
        int seconds=0, background=requested_bg;
        if(omikuji && *tail>='0' && *tail<='9') {
            while(*tail>='0' && *tail<='9') {
                seconds=seconds*10+(*tail++-'0');
                if(seconds>30) break;
            }
            while(*tail==' ') tail++;
            if(seconds<1 || seconds>30) { put("usage: omikuji [1-30] [&]\r\n"); return; }
        }
        if(*tail) { put(omikuji?"usage: omikuji [1-30] [&]\r\n":"usage: blink [&]\r\n"); return; }
        int pid=spawn(omikuji?3:1, (omikuji && background && !seconds ? 31 : seconds), 0);
        if(pid<0) { put("cannot start program (slot busy or invalid argument)\r\n"); return; }
        if(background) { *bg_pid=pid; *bg_name=omikuji?"omikuji":"blink"; }
        else {
            int status=0;
            set_foreground(pid);
            for(;;) {
                int rc=waitpid(pid, &status, 0);
                if(rc==pid) break;
                if(rc<0) { put("waitpid: error\r\n"); break; }
                sleep(10);
            }
            set_foreground(0);
        }
    } else if(eq(s,"trace") || prefix(s,"trace ")) {
        const char *arg=s+5;
        while(*arg==' ') arg++;
        if(!eq(arg,"on") && !eq(arg,"off") && !eq(arg,"show")) {
            put("usage: trace on|off|show\r\n"); return;
        }
        int pid=spawn(7,0,arg);
        if(pid<0) { put("trace: cannot start\r\n"); return; }
        if(requested_bg) { *bg_pid=pid; *bg_name="trace"; }
        else {
            int status=0;
            set_foreground(pid);
            while(waitpid(pid,&status,0)==0) sleep(10);
            set_foreground(0);
        }
    } else if(eq(s,"hello") || prefix(s,"hello ") ||
              eq(s,"hexdump") || prefix(s,"hexdump ")) {
        int is_dump=prefix(s,"hexdump");
        const char *arg=s+(is_dump?7:5);
        while(*arg==' ') arg++;
        char path[128];
        if(is_dump && *arg) {
            /* Resolve just the filename, preserving optional offset and count. */
            const char *suffix=arg;
            while(*suffix && *suffix!=' ') suffix++;
            char file[128];
            size_t len=(size_t)(suffix-arg);
            if(len>=sizeof file) { put("hexdump: path too long\r\n"); return; }
            memcpy(file,arg,len); file[len]=0;
            if(resolve(cwd,file,path)<0) {
                put("hexdump: path too long\r\n"); return;
            }
            len=strlen(path);
            if(len+strlen(suffix)>=sizeof path) {
                put("hexdump: arguments too long\r\n"); return;
            }
            strcpy(path+len,suffix);
            arg=path;
        }
        if(strlen(arg)>30) { put("argument too long\r\n"); return; }
        int pid=spawn(is_dump?6:5,0,*arg?arg:0);
        if(pid<0) { put("cannot start program\r\n"); return; }
        if(requested_bg) { *bg_pid=pid; *bg_name=is_dump?"hexdump":"hello"; }
        else {
            int status=0;
            set_foreground(pid);
            while(waitpid(pid,&status,0)==0) sleep(10);
            set_foreground(0);
        }
    } else if(prefix(s,"getty ")) {
        const char *arg=s+6;
        int tty=0;
        if(prefix(arg,"/dev/uart1 ")) {tty=2;arg+=11;}
        else if(prefix(arg,"/dev/console ")) {tty=1;arg+=13;}
        if(!tty || !(eq(arg,"sh") || eq(arg,"python"))) {
            put("usage: getty /dev/uart1 sh|python [&]\r\n");return;
        }
        int pid=tty_spawn(tty, eq(arg,"sh")?2:4);
        if(pid<0) {put("getty: terminal busy or no free slot\r\n");return;}
        if(requested_bg) {*bg_pid=pid;*bg_name=eq(arg,"sh")?"sh":"python";}
        else {int status=0;while(waitpid(pid, &status, 0)==0)sleep(10);}
    } else if(eq(s,"python") || prefix(s,"python ")) {
        char path[128];
        const char *arg=eq(s,"python")?0:s+7;
        if(arg) {
            while(*arg==' ') arg++;
            if(!*arg || resolve(cwd,arg,path)<0 || !prefix(path,"/tmp/") ||
               !path[5]) { put("python: invalid script path\r\n"); return; }
            if(strlen(path)>=32) { put("python: path too long\r\n"); return; }
        }
        int pid=spawn(4, 0, (arg?path:0));
        if(pid<0) { put("python: no free slot\r\n"); return; }
        if(requested_bg) { *bg_pid=pid; *bg_name="python"; }
        else {
            int status=0;
            set_foreground(pid);
            for(;;) {
                int rc=waitpid(pid, &status, 0);
                if(rc==pid || rc<0) break;
                sleep(10);
            }
            set_foreground(0);
        }
    } else if(eq(s,"ls") || prefix(s,"ls ")) {
        const char *arg=eq(s,"ls")?cwd:s+3;
        while(*arg==' ') arg++;
        if(!*arg) arg=cwd;
        char path[128];
        if(resolve(cwd,arg,path)<0) put("ls: path too long\r\n");
        else ls(path);
    }
    else if(prefix(s,"rm ")) {
        char path[128];
        if(resolve(cwd,s+3,path)<0) { put("rm: path too long\r\n"); return; }
        if(unlink(path)<0) put("rm: cannot remove\r\n");
    }
    else if(eq(s,"cat") || prefix(s,"cat ")) {
        char path[128];
        int from_stdin=eq(s,"cat");
        if(!from_stdin && resolve(cwd,trim(s+4),path)<0) { put("cat: path too long\r\n"); return; }
        if(!from_stdin && eq(path,"/dev/console")) { put("cat: console reserved for shell\r\n"); return; }
        int uart=!from_stdin && eq(path,"/dev/uart1");
        int fd=from_stdin?0:open(path, uart?4:0);
        if(fd<0) { put("cat: cannot open\r\n"); return; }
        // Proc reports can exceed one read. Read until EOF, using small
        // transfers to avoid overflowing the nonblocking USB console FIFO.
        if(from_stdin || prefix(path,"/proc/") || prefix(path,"/tmp/")) {
            char b[32]; int n;
            while((n=read(fd, b, sizeof b))>0) {
                write_all(1,b,(size_t)n);
            }
            if(n<0) put("cat: read error\r\n");
            else if(!from_stdin && !redirect_out) put("\r\n");
        } else if(uart) {
            /* UART is a byte stream without EOF. Poll in 10ms intervals,
               stop after 200ms of inactivity, including before first byte. */
            char b[32]; unsigned idle=0;
            while(idle<200) {
                int n=read(fd, b, sizeof b);
                if(n<0) { put("cat: read error\r\n"); break; }
                if(n>0) { write_all(1,b,(size_t)n); idle=0; }
                else { sleep(10); idle+=10; }
            }
            put("\r\n");
        } else {
            char b[32]; int n=read(fd, b, sizeof b);
            if(n>0) { write_all(1,b,(size_t)n); if(!redirect_out) put("\r\n"); }
            else if(n<0) put("cat: read error\r\n");
        }
        if(!from_stdin) close(fd);
    } else if(prefix(s,"echo ")) {
        put(s+5); put(redirect_out?"\n":"\r\n");
    } else put("unknown command (help)\r\n");
}
int main(int argc, char *argv[]) {
    char line[128];
    char cwd[128];
    cwd[0]='/';
    cwd[1]=0;
    /* USB CDC may enumerate after the shell task starts.  Do not discard the banner. */
    while(!console_ready()) sleep(10);
    put("\r\n--------------------------------------------------------\r\n"
        "PicoNIX-2026-10-26 (TAKAGO_LAB.)\r\n"
        "Type 'help' for commands.\r\n"
        "--------------------------------------------------------\r\n"
        "piconix# ");
    char chunk[128];
    int used=0, offset=0, length=0, previous_cr=0;
    for(;;) {
        int n=0;
        while(!n) {
            if(offset==length) {length=read(0, chunk, sizeof chunk);offset=0;
                if(length==0 && get_tty()==1) {
                    /* Ctrl+D ends this input, not the shell session. */
                    used=0; previous_cr=0; put("\r\npiconix# ");
                    continue;
                }
                if(length<=0) {sleep(1);continue;}}
            char c=chunk[offset++];
            if(c=='\n' && previous_cr) {previous_cr=0;continue;}
            previous_cr=(c=='\r');
            if(c=='\r' || c=='\n') {line[used]=0;n=1;used=0;break;}
            if(used<sizeof line-1) line[used++]=c;
        }
        /* Reap completed background children (no status display yet). */
        int bg_status;
        while(waitpid(-1, &bg_status, 0)>0) {}
        /* Parse '&' once for every command, before redirection. */
        char *cmd=trim(line);
        int requested_bg=0;
        size_t len=strlen(cmd);
        if(len && cmd[len-1]=='&') {
            cmd[len-1]=0;
            cmd=trim(cmd);
            requested_bg=1;
        }
        int saved_in=-1,saved_out=-1, bg_pid=0;
        const char *bg_name=0;
        int redir_ok=redirect(cmd,cwd,&saved_in,&saved_out);
        if(redir_ok==0) command(trim(cmd),cwd,saved_out>=0,requested_bg,&bg_pid,&bg_name);
        if(saved_in>=0) { dup2(saved_in, 0); close(saved_in); }
        if(saved_out>=0) { dup2(saved_out, 1); close(saved_out); }
        if(redir_ok<0) put("redirect: invalid path or cannot open file\r\n");
        /* Background notification must go to the console, not the file. */
        if(bg_pid>0) {
            put("["); printf("%d", bg_pid);
            put("] "); put(bg_name?bg_name:"program"); put(" started\r\n");
        }
        put("piconix# ");
    }
}
