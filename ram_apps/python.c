/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Tiny Python-like subset for PicoNIX: file execution and simple REPL. */
#include <stdint.h>
#include <limits.h>
#include <stddef.h>
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "stdio.h"
static void outn(const char *s,unsigned n) {
    while(n) { int r=write(1, s, n); if(r<0) return;
        if(!r) { msleep(1); continue; } s+=r;n-=(unsigned)r; }
}
static void outs(const char *s){outn(s,(unsigned)strlen(s));}
static int same(const char *a,const char *b){return strcmp(a,b)==0;}
static int idchar(char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';}
static char source[513];
static unsigned offsets[64],levels[64],count;
static struct {char name[16];int32_t val;} vars[16];
static int nvars,error;
static const char *cursor;
static void ws(void){while(*cursor==' '||*cursor=='\t')cursor++;}
static int eat(char c){ws();if(*cursor==c){cursor++;return 1;}return 0;}
static int word(const char *s){ws();const char *p=cursor;while(*s&&*p==*s){s++;p++;}if(*s||idchar(*p))return 0;cursor=p;return 1;}
static int32_t getvar(const char *name){for(int i=0;i<nvars;i++)if(same(vars[i].name,name))return vars[i].val;error=1;return 0;}
static void setvar(const char *name,int32_t val){for(int i=0;i<nvars;i++)if(same(vars[i].name,name)){vars[i].val=val;return;}
    if(nvars==16){error=1;return;}unsigned i=0;while(name[i]&&i<15){vars[nvars].name[i]=name[i];i++;}vars[nvars].name[i]=0;vars[nvars++].val=val;}
/* Keep the interpreter's divide-by-zero and INT32_MIN/-1 behavior explicit. */
static int32_t divide(int32_t a,int32_t b,int mod) {
    if(!b) { error=1; return 0; }
    /* The mathematical quotient 2147483648 does not fit in int32_t.
       Preserve the previous interpreter's 32-bit wraparound result. */
    if(a==INT32_MIN && b==-1) return mod ? 0 : INT32_MIN;
    return mod ? a%b : a/b;
}
/* GPIO accesses go through PicoNIX device files; no new syscall required. */
static int gpio_path(char path[20],int32_t pin,int direction){
    if(pin<0||pin>28||pin==0||pin==1||pin==4||pin==5){error=1;return 0;}
    const char *prefix="/dev/gpio/";unsigned n=0;
    while(prefix[n]){path[n]=prefix[n];n++;}
    if(pin>=20){path[n++]='2';pin-=20;}
    else if(pin>=10){path[n++]='1';pin-=10;}
    path[n++]=(char)('0'+pin);
    if(direction){path[n++]='.';path[n++]='d';path[n++]='i';path[n++]='r';}
    path[n]=0;return 1;
}
static int gpio_config(int32_t pin,int32_t value,int pull){
    char path[20];
    if(!gpio_path(path,pin,0))return 0;
    unsigned n=0;while(path[n])n++;
    const char *suffix=pull?".pull":".dir";
    for(unsigned i=0;suffix[i];i++)path[n++]=suffix[i];
    path[n]=0;
    const char *mode=pull?(value==0?"none":value==1?"up":"down"):
                          (value==0?"in":"out");
    if(value<0 || value>(pull?2:1)){error=1;return 0;}
    int fd=open(path, 1);
    if(fd<0){error=1;return 0;}
    unsigned len=0;while(mode[len])len++;
    int rc=write(fd, mode, len);
    close(fd);
    if(rc!=(int)len){error=1;return 0;}
    return 1;
}
static int gpio_output(int32_t pin,int32_t val){
    char path[20];if(!gpio_path(path,pin,1))return 0;
    if(val!=0&&val!=1){error=1;return 0;}
    int fd=open(path, 1);
    if(fd<0){error=1;return 0;}
    int n=write(fd, "out", 3);close(fd);
    if(n!=3){error=1;return 0;}
    if(!gpio_path(path,pin,0))return 0;
    fd=open(path, 1);
    if(fd<0){error=1;return 0;}
    char c=(char)('0'+val);
    n=write(fd, &c, 1);close(fd);
    if(n!=1){error=1;return 0;}return 1;
}
static int32_t gpio_input(int32_t pin){
    char path[20];if(!gpio_path(path,pin,0))return 0;
    int fd=open(path, 0);
    if(fd<0){error=1;return 0;}
    char c=0;int n=read(fd, &c, 1);
    close(fd);
    if(n!=1||(c!='0'&&c!='1')){error=1;return 0;}
    return c-'0';
}
/* Build a line from byte-stream UART reads as well as console reads. */
static int read_line(char *buf,unsigned cap){
    static int skip_lf;
    unsigned n=0;
    for(;;){
        char c;
        int r=read(0, &c, 1);
        if(r<=0)return n?(int)n:-1;
        if(skip_lf){skip_lf=0;if(c=='\n')continue;}
        if(c=='\r'){skip_lf=1;break;}
        if(c=='\n')break;
        if(n+1<cap)buf[n++]=c;
    }
    buf[n]=0;
    return (int)n;
}
static int32_t expr(void);
static int32_t atom(void){ws();if(eat('(')){int32_t x=expr();if(!eat(')'))error=1;return x;}
    if(eat('-'))return (int32_t)(0u-(uint32_t)atom());
    if(eat('+'))return atom();
    if(*cursor>='0'&&*cursor<='9'){uint32_t v=0;while(*cursor>='0'&&*cursor<='9'){v=v*10u+(unsigned)(*cursor++-'0');}return (int32_t)v;}
    char name[16];unsigned n=0;
    while(idchar(*cursor)&&n<15)name[n++]=*cursor++;
    name[n]=0;if(!n){error=1;return 0;}
    if(same(name,"input")&&eat('(')){
        ws();if(*cursor=='"'||*cursor=='\''){char quote=*cursor++;const char *start=cursor;
            while(*cursor&&*cursor!=quote)cursor++;outn(start,(unsigned)(cursor-start));if(!eat(quote))error=1;}
        if(!eat(')'))error=1;
        char buf[32];int m=read_line(buf,sizeof(buf));int32_t v=0,sign=1;unsigned i=0;
        if(m<=0){error=1;return 0;}if(buf[0]=='-'){sign=-1;i++;}
        int digits=0;for(;i<(unsigned)m && buf[i]>='0'&&buf[i]<='9';i++){v=(int32_t)((uint32_t)v*10u+(unsigned)(buf[i]-'0'));digits++;}
        if(!digits)error=1;return sign<0?(int32_t)(0u-(uint32_t)v):v;
    }
    if(same(name,"gpio_read")&&eat('(')){
        int32_t pin=expr();if(!eat(')')){error=1;return 0;}
        return gpio_input(pin);
    }
    if(same(name,"int")&&eat('(')){int32_t v=expr();if(!eat(')'))error=1;return v;}
    return getvar(name);
}
static int32_t product(void){int32_t a=atom();for(;;){if(eat('*'))a=(int32_t)((uint32_t)a*(uint32_t)atom());
    else if(eat('/'))a=divide(a,atom(),0);else if(eat('%'))a=divide(a,atom(),1);else break;}return a;}
static int32_t sum(void){int32_t a=product();for(;;){if(eat('+'))a=(int32_t)((uint32_t)a+(uint32_t)product());
    else if(eat('-'))a=(int32_t)((uint32_t)a-(uint32_t)product());else break;}return a;}
static int32_t expr(void){int32_t a=sum();ws();int op=0;
    if(cursor[0]=='='&&cursor[1]=='='){op=1;cursor+=2;}
    else if(cursor[0]=='!'&&cursor[1]=='='){op=2;cursor+=2;}
    else if(cursor[0]=='<'&&cursor[1]=='='){op=3;cursor+=2;}
    else if(cursor[0]=='>'&&cursor[1]=='='){op=4;cursor+=2;}
    else if(eat('<'))op=5;else if(eat('>'))op=6;
    if(op){int32_t b=sum();
        if(op==1)return a==b;
        if(op==2)return a!=b;
        if(op==3)return a<=b;
        if(op==4)return a>=b;
        if(op==5)return a<b;
        return a>b;
    }
    return a;
}
static void number(int32_t v){char b[12];unsigned n=0,x=v<0?0u-(uint32_t)v:(uint32_t)v;
    do{uint32_t r=x%10u;x/=10u;b[n++]=(char)('0'+r);}while(x&&n<11);
    if(v<0)b[n++]='-';while(n){char c=b[--n];outn(&c,1);}}
static int repl_mode;
static void statement(unsigned line){cursor=source+offsets[line];ws();
    if(word("msleep")&&eat('(')){
        int32_t ms=expr();if(!eat(')'))error=1;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(ms<0||ms>60000)error=1;
        if(!error)msleep(ms);
        return;
    }
    cursor=source+offsets[line];ws();
    if(word("gpio")&&eat('(')){
        int32_t pin=expr();if(!eat(','))error=1;
        int32_t val=expr();if(!eat(')'))error=1;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(!error)gpio_output(pin,val);
        return;
    }
    cursor=source+offsets[line];ws();
    int pull=0,mode=0;
    if((pull=word("gpio_pull")) || (mode=word("gpio_mode"))) {
        if(!eat('(')){error=1;return;}
        int32_t pin=expr();if(!eat(','))error=1;
        int32_t val=expr();if(!eat(')'))error=1;
        ws();if(*cursor&&*cursor!='#')error=1;
        if(!error)gpio_config(pin,val,pull);
        return;
    }
    cursor=source+offsets[line];ws();
    if(word("print")&&eat('(')){
        ws();if(*cursor=='\''||*cursor=='"'){char q=*cursor++;const char *start=cursor;
            while(*cursor&&*cursor!=q)cursor++;outn(start,(unsigned)(cursor-start));if(!eat(q))error=1;}
        else number(expr());if(!eat(')'))error=1;
        ws();if(*cursor&&*cursor!='#')error=1;outs("\r\n");return;
    }
    cursor=source+offsets[line];ws();char name[16];unsigned n=0;
    if(!((*cursor>='a'&&*cursor<='z')||(*cursor>='A'&&*cursor<='Z')||*cursor=='_')){
        if(repl_mode){cursor=source+offsets[line];number(expr());outs("\r\n");return;}
        error=1;return;
    }
    while(idchar(*cursor)&&n<15)name[n++]=*cursor++;name[n]=0;
    if(!eat('=')){
        if(repl_mode){cursor=source+offsets[line];int32_t v=expr();
            ws();if(*cursor&&*cursor!='#')error=1;
            if(!error){number(v);outs("\r\n");}return;}
        error=1;return;
    }int32_t v=expr();ws();if(*cursor&&*cursor!='#')error=1;if(!error)setvar(name,v);
}
static unsigned block_end(unsigned start){unsigned i=start+1;while(i<count&&levels[i]>levels[start])i++;return i;}
static void execute(unsigned first,unsigned last,unsigned depth){if(depth>8){error=1;return;}
    for(unsigned i=first;i<last&&!error;){cursor=source+offsets[i];ws();
        if(word("if")||word("while")){int loop=source[offsets[i]]=='w';
            int32_t condition=expr();if(!eat(':')){error=1;break;}
            unsigned end=block_end(i);if(i+1==end||levels[i+1]<=levels[i]){error=1;break;}
            unsigned other=end;
            if(!loop&&end<last&&levels[end]==levels[i]){
                cursor=source+offsets[end];if(word("else")&&eat(':'))other=block_end(end);
            }
            if(loop){unsigned iterations=0;while(condition&&!error){if(++iterations>10000){error=1;break;}
                execute(i+1,end,depth+1);cursor=source+offsets[i];word("while");condition=expr();}}
            else if(condition)execute(i+1,end,depth+1);
            else if(other>end)execute(end+1,other,depth+1);
            i=other;
        } else { statement(i);i++; }
    }
}
static void run_source(unsigned n){
    count=0;error=0;
    unsigned off=0;
    while(off<n&&count<64){unsigned begin=off,indent=0;
        while(off<n && source[off]==' '){off++;indent++;}
        if(off<n && source[off]=='\t'){error=1;break;}
        unsigned content=off;
        while(off<n && source[off]!='\n' && source[off]!='\r')off++;
        unsigned end=off;
        while(end>content&&source[end-1]==' ')end--;
        if(content<end&&source[content]!='#'){
            offsets[count]=content;levels[count]=indent;count++;
        }
        while(off<n&&(source[off]=='\n'||source[off]=='\r'))off++;
        source[end]=0;
        if(off==begin)break;
    }
    if(!error)execute(0,count,0);
    if(error)outs("python: syntax/runtime error\r\n");
}
static void repl(void){
    outs("PicoNIX Tiny Python (Python subset)\r\n");
    outs("Type exit() to quit.\r\n");
    repl_mode=1;
    for(;;){
        unsigned n=0;int block=0;
        for(;;){
            outs(block?"... ":">>> ");
            char line[128];
            int m=read_line(line,sizeof(line));
            if(m<0){n=0;msleep(10);break;} /* EOF discards pending input, not the REPL. */
            while(m>0&&(line[m-1]=='\n'||line[m-1]=='\r'))m--;
            line[m]=0;
            if(!block && same(line,"exit()")){
                if(get_tty()==1)outs("PicoNIX: Session closed.\r\n");
                return;
            }
            if(!block && !m)break;
            if(block && !m)break;
            if(n+(unsigned)m+1>=sizeof(source)){
                outs("python: input too long\r\n");n=0;break;
            }
            for(int i=0;i<m;i++)source[n++]=line[i];
            source[n++]='\n';
            if(!block){
                unsigned i=0;while(line[i])i++;
                if(i && line[i-1]==':')block=1;
                else break;
            }
        }
        if(n){source[n]=0;run_source(n);}
    }
}
int main(int argc, char *argv[]){
    if(argc<2 || !argv[1]){repl();goto done;}
    const char *path=argv[1];
    int fd=open(path, 0);
    if(fd<0){outs("python: script not found\r\n");goto done;}
    int n=read(fd, source, 512);
    close(fd);
    if(n<0){outs("python: read error\r\n");goto done;}
    source[n]=0;run_source((unsigned)n);
done:
    _exit(error?1:0);
    for(;;)msleep(1000);
}
