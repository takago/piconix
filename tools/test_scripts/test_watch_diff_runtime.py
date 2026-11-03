"""Exercise watch's real frame renderer and pipe capture under host syscall stubs."""
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
cc = shutil.which('gcc') or shutil.which('clang')
assert cc, 'host C compiler is required'
code = r'''
#define main watch_app_main
#include "watch.c"
#undef main

static char terminal[16384];
static unsigned terminal_used;
static unsigned char source_data[3072];
static unsigned source_used, source_pos;
static int order_read, order_wait, saved_stdout, saved_stderr;
static int n_dup2, n_closed, child_pid;
extern int vsnprintf(char *, size_t, const char *, va_list);
int printf(const char *fmt, ...) {
    va_list args;va_start(args,fmt);
    int n=vsnprintf(terminal+terminal_used,sizeof terminal-terminal_used,fmt,args);
    va_end(args);
    if(n<0 || (unsigned)n>=sizeof terminal-terminal_used) return -1;
    terminal_used+=(unsigned)n;
    return n;
}
int puts(const char *s) {return printf("%s\n",s);}
int pipe(int fds[2]) { fds[0]=5;fds[1]=6;return 0; }
int dup(int fd) { return fd==1?7:8; }
int dup2(int src,int dest) {
    if(src==7 && dest==1) saved_stdout=1;
    if(src==8 && dest==2) saved_stderr=1;
    ++n_dup2;return dest;
}
int close(int fd) { (void)fd;++n_closed;return 0; }
int spawn(int id,int argc,const char *const argv[]) {
    (void)id;(void)argc;(void)argv;
    child_pid=42;return child_pid;
}
int read(int fd,void *p,size_t count) {
    if(fd!=5) return -1;
    ++order_read;
    if(source_pos>=source_used) return 0;
    if(count>source_used-source_pos) count=source_used-source_pos;
    memcpy(p,source_data+source_pos,count);
    source_pos+=(unsigned)count;
    return (int)count;
}
int waitpid(int pid,int *status,int options) {
    (void)status;(void)options;
    if(pid!=42 || !order_read) return -1;
    ++order_wait;return pid;
}
int msleep(unsigned n) {(void)n;return 0;}
int program_id(const char *s) {(void)s;return 1;}
#define CHECK(x) do {if(!(x))return __LINE__;} while(0)
static void clear_terminal(void) {terminal_used=0;terminal[0]=0;}
static void load(const char *s) {
    source_used=(unsigned)strlen(s); source_pos=0;
    memcpy(source_data,s,source_used);
}
int main(void) {
    watch_frame_t a={0},b={0};
    const char *argv[]={"ps"};
    load("alpha\nbeta\n");
    CHECK(capture_command(8,1,argv,&a)==0);
    CHECK(!strcmp(a.line[0],"alpha") && !strcmp(a.line[1],"beta"));
    CHECK(saved_stdout && saved_stderr && n_dup2==4 && n_closed==4);
    CHECK(order_read>0 && order_wait==1);
    /* A command can exceed the 256-byte kernel pipe; drain to EOF. */
    for(int i=0;i<1024;i++)source_data[i]='Q';
    source_used=1024;source_pos=0;
    watch_frame_t large={0};
    CHECK(capture_command(8,1,argv,&large)==0);
    CHECK(source_pos==1024 && large.truncated);
    clear_terminal();redraw(&b,&a,1);
    CHECK(strstr(terminal,"\033[3;1Halpha\033[K")!=0);
    CHECK(strstr(terminal,"\033[?7l\033[3;1H")!=0);
    CHECK(strstr(terminal,"\033[K\033[?7h")!=0);
    clear_terminal();redraw(&a,&a,0);
    CHECK(terminal_used==0); /* unchanged rows produce zero output */
    capture_byte(&b,'a');capture_byte(&b,'l');capture_byte(&b,'p');
    capture_byte(&b,'h');capture_byte(&b,'a');capture_byte(&b,'\n');
    capture_byte(&b,'B');capture_byte(&b,'\n');
    clear_terminal();redraw(&a,&b,0);
    CHECK(!strstr(terminal,"\033[3;1H"));
    CHECK(strstr(terminal,"\033[4;1HB\033[K"));
    watch_frame_t c={0};
    clear_terminal();redraw(&b,&c,0);
    CHECK(strstr(terminal,"\033[3;1H\033[K"));
    CHECK(strstr(terminal,"\033[4;1H\033[K"));
    watch_frame_t d={0};
    const char *escaped="hello\033[31m!\033[0m\rbye\nnext\n";
    for(const char *p=escaped;*p;p++)capture_byte(&d,(unsigned char)*p);
    CHECK(!strcmp(d.line[0],"bye") && !strcmp(d.line[1],"next"));
    watch_frame_t e={0};
    for(int i=0;i<3000;i++)capture_byte(&e,'x');
    CHECK(e.truncated && strlen(e.line[0])==WATCH_COLS);
    watch_frame_t utf8={0};
    const char *japanese="\343\201\202"; /* Japanese hiragana 'a' */
    for(const char *p=japanese;*p;p++)capture_byte(&utf8,(unsigned char)*p);
    CHECK(!strcmp(utf8.line[0],japanese));
    watch_frame_t f={0};
    for(int i=0;i<WATCH_ROWS+3;i++){
        capture_byte(&f,'y');capture_byte(&f,'\n');
    }
    CHECK(f.truncated && f.rows==WATCH_ROWS);
    clear_terminal();redraw(&a,&e,0);
    CHECK(strstr(terminal,"[watch: output truncated]"));
    return 0;
}
'''
with tempfile.TemporaryDirectory() as temp:
    cfile = Path(temp) / 'watch_test.c'
    binary = Path(temp) / 'watch_test'
    cfile.write_text(code)
    subprocess.run([cc, '-std=c11','-DPICONIX_HOST_TEST', '-O1', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'userland' / 'lib'), '-I', str(root / 'userland' / 'apps'), '-o', str(binary), str(cfile)],
                   cwd=root / 'tools/test_scripts', check=True)
    proc = subprocess.run([str(binary)], capture_output=True, text=True)
    assert proc.returncode == 0, f'watch runtime test failed: line {proc.returncode}\n{proc.stderr}'
print('PASS: watch frame diff, pipe draining, stdout/stderr restore and bounds')
