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
        if(rc==0) { msleep(1); continue; }
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
    char *first=0;
    for(char *q=s;*q;q++) if(*q=='<' || *q=='>') {
        if(!first) first=q;
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
    if(prefix(name,"./") || prefix(name,"/tmp/")) {
        char path[128];
        if(argc!=1 || resolve(cwd,name,path)<0) { put("invalid executable path\r\n"); return 1; }
        int pid=spawn_tmp(path);
        if(pid<0) { put("cannot execute image\r\n"); return 1; }
        if(background) {*bg_pid=pid;*bg_name="a.out";}
        else { int status=0; set_foreground(pid); waitpid(pid,&status,0); set_foreground(0); }
        return 1;
    }
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
        /* Full-screen programs may be killed before restoring the screen. */
        if(eq(name,"mb") || eq(name,"gpio") || eq(name,"pong") ||
           eq(name,"fbed") || eq(name,"edit") || eq(name,"mem"))
            put("\033[?1049l");
    }
    return 1;
}
static void command(char *s, char cwd[128], int requested_bg, int *bg_pid, const char **bg_name) {
    while(*s==' ') s++;
    if(!*s) return;
    if(eq(s,"logout") || eq(s,"exit")) {
        put("logout\r\nPicoNIX: Session closed.\r\n");
        _exit(0);
        for(;;) msleep(100);
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
/* Compact per-shell history; no heap allocation or kernel changes. */
#define HISTORY_SIZE 8
#define SHELL_PROMPT "piconix# "
static char history[HISTORY_SIZE][128];
static unsigned history_count, history_next;

static void history_add(const char *line) {
    if(!*line) return;
    if(history_count && eq(history[(history_next+HISTORY_SIZE-1)%HISTORY_SIZE],line)) return;
    strcpy(history[history_next],line);
    history_next=(history_next+1)%HISTORY_SIZE;
    if(history_count<HISTORY_SIZE) history_count++;
}
/* Move left from the current position by n columns. */
static void shell_move_left(unsigned n) {
    if(!n) return;
    char digits[4];unsigned used=0;
    do {digits[used++]=(char)('0'+n%10);n/=10;} while(n);
    put("\033[");
    while(used) write_all(1,&digits[--used],1);
    put("D");
}
/* Terminal tab stops are every four columns, including the prompt. */
static unsigned shell_columns(const char *line,unsigned len) {
    unsigned col=sizeof(SHELL_PROMPT)-1;
    for(unsigned i=0;i<len;i++)col+=line[i]=='\t'?4-(col&3u):1;
    return col;
}
static int shell_has_tab(const char *line,unsigned len) {
    for(unsigned i=0;i<len;i++)if(line[i]=='\t')return 1;
    return 0;
}
static void shell_print_expanded(const char *line,unsigned len) {
    unsigned col=sizeof(SHELL_PROMPT)-1;
    for(unsigned i=0;i<len;i++) {
        if(line[i]=='\t') {
            unsigned n=4-(col&3u);
            for(unsigned j=0;j<n;j++)put(" ");
            col+=n;
        }else {write_all(1,line+i,1);col++;}
    }
}
/* Redraw a line with four-column tab expansion. */
static void shell_redraw(const char *line, unsigned len, unsigned cursor) {
    put("\r" SHELL_PROMPT);
    shell_print_expanded(line,len);
    put("\033[K");
    shell_move_left(shell_columns(line,len)-shell_columns(line,cursor));
}
/* Update from the edited column, leaving the prompt and prefix alone. */
static void shell_redraw_tail(const char *line, unsigned len, unsigned cursor) {
    write_all(1,line+cursor,len-cursor);
    put("\033[K");
    shell_move_left(len-cursor);
}
/* Read one line in raw/no-echo mode; restore the TTY before commands run.
 * If this TTY cannot be controlled, keep the original canonical fallback. */
static int shell_readline(char line[128]) {
    int canon=ioctl(0,OS_TTY_GET_CANON,0);
    int echo=ioctl(0,OS_TTY_GET_ECHO,0);
    if(canon<0 || echo<0 || ioctl(0,OS_TTY_SET_CANON,0)<0 ||
       ioctl(0,OS_TTY_SET_ECHO,0)<0) {
        if(canon>=0) ioctl(0,OS_TTY_SET_CANON,canon);
        if(echo>=0) ioctl(0,OS_TTY_SET_ECHO,echo);
        unsigned used=0; int previous_cr=0;
        for(;;) {
            char c;int n=read(0,&c,1);
            if(n<=0) {if(n==0 && get_tty()==1) {put("\r\n" SHELL_PROMPT);used=0;} else msleep(1);continue;}
            if(c=='\n' && previous_cr) {previous_cr=0;continue;}
            previous_cr=(c=='\r');
            if(c=='\r'||c=='\n') {line[used]=0;return 1;}
            if(used<127) line[used++]=c;
        }
    }
    char draft[128]={0};
    line[0]=0;
    unsigned len=0,cursor=0;
    unsigned selected=history_count; /* history_count means draft */
    int escape=0;
    for(;;) {
        char c;
        int n=read(0,&c,1);
        if(n<=0) {if(n<0) msleep(1);else msleep(1);continue;}
        if(escape==1) {escape=(c=='[' || c=='O')?2:0;continue;}
        if(escape==2) {
            escape=0;
            if(c=='A' || c=='B') {
                if(c=='A' && selected==0) continue;
                if(c=='B' && selected==history_count) continue;
                if(selected==history_count) strcpy(draft,line);
                if(c=='A') selected--;
                else selected++;
                if(selected==history_count) strcpy(line,draft);
                else strcpy(line,history[(history_next+HISTORY_SIZE-history_count+selected)%HISTORY_SIZE]);
                len=(unsigned)strlen(line);cursor=len;
            } else if(c=='C') {
                if(cursor<len) {cursor++;if(shell_has_tab(line,len))shell_redraw(line,len,cursor);else put("\033[C");}
                continue;
            } else if(c=='D') {
                if(cursor) {cursor--;if(shell_has_tab(line,len))shell_redraw(line,len,cursor);else put("\033[D");}
                continue;
            } else continue;
            shell_redraw(line,len,cursor);
            continue;
        }
        if(c==12) { /* Ctrl+L: clear screen, preserve the edited line and cursor. */
            put("\033[2J\033[H");
            shell_redraw(line,len,cursor);
            continue;
        }
        if(c==27) {escape=1;continue;}
        if(c=='\r' || c=='\n') {
            line[len]=0;
            put("\r\n");
            history_add(line);
            ioctl(0,OS_TTY_SET_ECHO,echo);
            ioctl(0,OS_TTY_SET_CANON,canon);
            return 1;
        }
        if(c==8 || c==127) {
            if(!cursor) continue;
            int had_tab=shell_has_tab(line,len);
            for(unsigned i=cursor;i<len;i++)line[i-1]=line[i];
            len--;cursor--;line[len]=0;
            if(had_tab) shell_redraw(line,len,cursor);
            else {
                put("\b");
                if(cursor==len) put(" \b");
                else shell_redraw_tail(line,len,cursor);
            }
        } else if((c=='\t' || (c>=32 && c<=126)) && len<127) {
            int at_end=(cursor==len);
            for(unsigned i=len;i>cursor;i--)line[i]=line[i-1];
            line[cursor++]=c;line[++len]=0;
            if(shell_has_tab(line,len)) shell_redraw(line,len,cursor);
            else if(at_end) write_all(1,&c,1);
            else {
                /* Cursor is still at the insertion point on the terminal. */
                shell_redraw_tail(line,len,cursor-1);
                put("\033[C");
            }
        }
    }
}
int main(int argc, char *argv[]) {
    char line[128];
    char cwd[128];
    if(chdir("/tmp")<0) {
        put("sh: cannot cd to /tmp\r\n");
        _exit(1);
    }
    strcpy(cwd,"/tmp");
    /* USB CDC may enumerate after the shell task starts.  Do not discard the banner. */
    while(!console_ready()) msleep(10);
    put("\r\n--------------------------------------------------------\r\n"
        "PicoNIX-2026-10-30 (TAKAGO_LAB.)\r\n"
        "Type 'help' for commands.\r\n"
        "--------------------------------------------------------\r\n"
        "\033[0m\033[?25h" SHELL_PROMPT);
    for(;;) {
        shell_readline(line);
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
        put("\033[0m\033[?25h" SHELL_PROMPT);
    }
}
