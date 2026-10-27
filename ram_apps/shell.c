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
/* /bin listing is the executable registry; IDs follow the registration order. */
static int program_id(const char *name) {
    /* /bin lists registered commands, one per line, in program ID order. */
    int fd=open("/bin",0);
    if(fd<0) return -1;
    char line[32],buf[64];
    int used=0,id=1,found=-1,n;
    while((n=read(fd,buf,sizeof buf))>0) {
        for(int i=0;i<n;i++) {
            char c=buf[i];
            if(c=='\n') {
                line[used]=0;
                if(eq(name,line)) {found=id;break;}
                id++;used=0;
            } else if(used<(int)sizeof line-1) line[used++]=c;
        }
        if(found>=0) break;
    }
    close(fd);
    return found;
}
static int run_external(char *cmd,const char *cwd,int background,int *bg_pid,const char **bg_name) {
    char *args[8];int argc=0;
    char *q=cmd;
    while(*q) {
        while(*q==' ' || *q=='\t') q++;
        if(!*q) break;
        if(argc==8) {put("too many arguments (max 8)\r\n");return 1;}
        args[argc++]=q;
        while(*q && *q!=' ' && *q!='\t') q++;
        if(*q) *q++=0;
    }
    if(!argc) return 1;
    const char *name=args[0];
    if(prefix(name,"/bin/")) name+=5;
    int id=program_id(name);
    if(id<0) return 0;
    args[0]=(char *)name;
    char path[128];
    if(id==3 && background && argc==1) {
        args[argc++]="31"; /* Original noninteractive Omikuji default. */
    }
    if((id==4 || id==6 || id==9) && argc>1) {
        if(resolve(cwd,args[1],path)<0) {put("invalid path\r\n");return 1;}
        args[1]=path;
    }
    unsigned total=0;
    for(int i=0;i<argc;i++) total+=(unsigned)strlen(args[i])+1;
    if(total>128) {put("arguments too long (max 128 bytes)\r\n");return 1;}
    int pid=spawn(id,argc,(const char *const *)args);
    if(pid<0) {put("cannot start program\r\n");return 1;}
    if(background) {*bg_pid=pid;*bg_name="program";}
    else {
        int status=0;
        set_foreground(pid);
        waitpid(pid,&status,0);
        set_foreground(0);
    }
    return 1;
}
static void command(char *s, char cwd[128], int requested_bg, int *bg_pid, const char **bg_name) {
    while(*s==' ') s++;
    if(!*s) return;
    if(eq(s,"help")) {
        put("PicoNIX command help\r\n"
            "\r\nTask management:\r\n"
            "  help             Show this help\r\n"
            "  ps               Show tasks and RAM layout (external)\r\n"
                        "  tty              Show controlling terminal (external)\r\n"
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
            "  pwd              Print current directory (external)\r\n"
            "  cd [PATH]        Change directory (default /)\r\n"
            "  ls [PATH]        List current or specified directory\r\n"
            "  ls /bin          List external programs and Shell built-ins\r\n"
            "  /bin/hello NAME  Execute using an absolute command path\r\n"
            "  ls /dev          List devices\r\n"
            "  ls /dev/gpio     List GPIO pins (N, N.dir, N.pull)\r\n"
            "  ls /proc     List kernel statistics\r\n"
            "  cat PATH         Read file or device (external)\r\n"
            "  cat > PATH       Enter file contents; Ctrl+D ends input\r\n"
            "  watch [N] CMD    Repeat external command every N seconds\r\n"
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
    else if(eq(s,"cd") || prefix(s,"cd ")) {
        const char *arg=eq(s,"cd")?"/":s+3;
        while(*arg==' ') arg++;
        if(!*arg) arg="/";
        char path[128];
        if(resolve(cwd,arg,path)<0 || chdir(path)<0) { put("cd: no such directory\r\n"); return; }
        strcpy(cwd,path);
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
        else {int status=0;waitpid(pid, &status, 0);}
    } else if(!run_external(s,cwd,requested_bg,bg_pid,bg_name))
        put("unknown command (help)\r\n");
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
        while(waitpid(-1, &bg_status, 1)>0) {}
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
        if(redir_ok==0) command(trim(cmd),cwd,requested_bg,&bg_pid,&bg_name);
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
