/* Small two-pass Thumb-1 assembler for PicoNIX (educational subset). */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>
#include "stdlib.h"

#define MAX_SRC 1024
#define MAX_OUT 1016
#define MAX_LABELS 32
static char src[MAX_SRC+1];
static uint8_t code[MAX_OUT];
static struct { char name[24]; unsigned offset; } labels[MAX_LABELS];
static int nlabels;
static unsigned pos;
static int err;

static char *space(char *s) { while(*s==' '||*s=='\t') ++s; return s; }
static void end(char *s) { char *p=s+strlen(s); while(p>s && (p[-1]==' '||p[-1]=='\t'||p[-1]=='\r')) *--p=0; }
static int number(char *s,int *v) {
    s=space(s); if(*s=='#') s++;
    int sign=1,base=10,result=0,digits=0;
    if(*s=='-') {sign=-1;s++;}
    if(s[0]=='0' && (s[1]=='x'||s[1]=='X')) {s+=2;base=16;}
    while(*s) {int d=(*s>='0'&&*s<='9')?*s-'0':(*s>='a'&&*s<='f')?*s-'a'+10:(*s>='A'&&*s<='F')?*s-'A'+10:-1;
        if(d<0||d>=base) break; result=result*base+d;digits++;s++;}
    if(!digits||*space(s)) return 0;
    *v=sign*result;return 1;
}
static int reg(char *s) {
    s=space(s); if(*s!='r'||s[1]<'0'||s[1]>'7'||*space(s+2))return -1;
    return s[1]-'0';
}
static int registers(char *s,int pop) {
    s=space(s);if(*s++!='{')return -1;
    int mask=0;
    while(1){
        s=space(s);char *start=s;
        while(*s && *s!=',' && *s!='}')s++;
        if(!*s)return -1;
        char delim=*s;*s=0;end(start);
        int r=reg(start);
        if(r>=0){if(mask&(1<<r))return -1;mask|=1<<r;}
        else if((pop && !strcmp(start,"pc"))||(!pop && !strcmp(start,"lr"))){if(mask&256)return -1;mask|=256;}
        else return -1;
        if(delim=='}')return *space(s+1)?-1:mask;
        s++;
    }
}
static int memory_operand(char *s,int *base,int *offset) {
    s=space(s);if(*s++!='[')return 0;
    char *e=strchr(s,']');if(!e||*space(e+1))return 0;
    *e=0;char *c=strchr(s,',');
    if(c){*c++=0;if(!number(c,offset))return 0;}
    else *offset=0;
    *base=reg(s);return *base>=0;
}
static int label(char *s) {for(int i=0;i<nlabels;i++)if(!strcmp(labels[i].name,s))return (int)labels[i].offset;return -1;}
static void emit16(unsigned v,int pass) {if(pos+2>MAX_OUT){err=1;return;} if(pass){code[pos]=(uint8_t)v;code[pos+1]=(uint8_t)(v>>8);} pos+=2;}
static void byte(unsigned v,int pass) {if(pos>=MAX_OUT){err=1;return;} if(pass)code[pos]=(uint8_t)v;pos++;}
static int compile(int pass) {
    pos=0;err=0;char text[MAX_SRC+1];strcpy(text,src);
    char *line=text;
    while(*line && !err) {
        char *next=strchr(line,'\n');if(next)*next++=0;
        char *p=space(line);char *comment=strchr(p,'@');if(comment)*comment=0;
        comment=strchr(p,';');if(comment)*comment=0;end(p);
        if(*p) {
            char *colon=strchr(p,':');
            if(colon) { *colon=0;end(p);if(!pass) {
                    if(!*p||strlen(p)>=sizeof labels[0].name||nlabels>=MAX_LABELS||label(p)>=0){err=1;break;}
                    strcpy(labels[nlabels].name,p);labels[nlabels++].offset=pos;
                }p=space(colon+1); }
            if(*p) {
                char *arg=p;while(*arg && *arg!=' ' && *arg!='\t')arg++;
                if(*arg)*arg++=0;arg=space(arg);end(arg);
                unsigned ins=0;int v=0,r=-1;char *comma=strchr(arg,',');
                if(!strcmp(p,".ascii")) {
                    if(*arg!='\"'){err=1;break;}char *q=arg+1;
                    while(*q && *q!='\"') {
                        unsigned ch=(uint8_t)*q++;
                        if(ch=='\\') {if(!*q){err=1;break;}ch=(*q=='n')?'\n':(*q=='r')?'\r':(*q=='t')?'\t':(uint8_t)*q;q++;}
                        byte(ch,pass);
                    }
                    if(*q!='\"'||*space(q+1))err=1;
                    if(next){line=next;continue;}break;
                }
                if(!strcmp(p,".byte")||!strcmp(p,".word")||!strcmp(p,".space")) {
                    if(!number(arg,&v)){err=1;break;}
                    if(!strcmp(p,".space")){if(v<0||v>MAX_OUT)err=1;else for(int j=0;j<v;j++)byte(0,pass);}
                    else if(!strcmp(p,".byte")){if(v<0||v>255)err=1;else byte((unsigned)v,pass);}
                    else {if(pos&3)err=1;else for(int j=0;j<4;j++)byte(((uint32_t)v)>>(8*j),pass);}
                    if(next){line=next;continue;}break;
                }
                if(!strcmp(p,".align")) {if(strcmp(arg,"2")){err=1;break;}while(pos&3)byte(0,pass);if(next){line=next;continue;}break;}
                if(!strcmp(p,"nop")) {if(*arg)err=1;ins=0xbf00;}
                else if(!strcmp(p,"bx")) {if(strcmp(arg,"lr"))err=1;ins=0x4770;}
                else if(!strcmp(p,"push")||!strcmp(p,"pop")) {
                    int pop=!strcmp(p,"pop"), mask=registers(arg,pop);
                    if(mask<=0)err=1;
                    ins=(pop?0xbc00:0xb400)|(mask&255)|((mask&256)?0x100:0);
                }
                else if(!strcmp(p,"bl")) {
                    int target=label(arg);if(target<0){if(pass)err=1;target=(int)pos+4;}
                    int delta=target-(int)pos-4;
                    if(pass && ((pos&1)||(delta&1)||delta < -2048 || delta > 2046))err=1;
                    uint32_t imm=(uint32_t)delta;
                    emit16(0xf000|((imm>>12)&0x7ff),pass);
                    emit16(0xf800|((imm>>1)&0x7ff),pass);
                    if(next){line=next;continue;}break;
                }
                else if(!strcmp(p,"svc")) {if(!number(arg,&v)||v<0||v>255)err=1;ins=0xdf00|(v&255);}
                else if(!strcmp(p,"b")||!strcmp(p,"beq")||!strcmp(p,"bne")||!strcmp(p,"bcc")||!strcmp(p,"bcs")) {
                    int target=label(arg);if(target<0){if(pass)err=1;target=(int)pos+4;}
                    int delta=target-(int)pos-4;int conditional=strcmp(p,"b")!=0;
                    if(pass && ((delta&1)||delta < (conditional?-256:-2048) || delta>(conditional?254:2046)))err=1;
                    ins=conditional?(0xd000|(!strcmp(p,"bne")?0x100:!strcmp(p,"bcs")?0x200:!strcmp(p,"bcc")?0x300:0)|((delta>>1)&255)):(0xe000|((delta>>1)&2047));
                } else {
                    if(!comma){err=1;break;}*comma++=0;end(arg);comma=space(comma);
                    r=reg(arg);if(r<0){err=1;break;}
                    if(!strcmp(p,"mov")) {
                        int other=reg(comma);if(other<0)err=1;
                        ins=0x4600|((r&8)<<4)|((other&15)<<3)|(r&7);
                    } else if(!strcmp(p,"ldr")||!strcmp(p,"str")||!strcmp(p,"ldrb")||!strcmp(p,"strb")) {
                        int base,off;
                        if(!memory_operand(comma,&base,&off)||off<0)err=1;
                        else {
                            int byteop=(!strcmp(p,"ldrb")||!strcmp(p,"strb"));
                            if((!byteop && (off&3))||off>(byteop?31:124))err=1;
                            unsigned op=!strcmp(p,"str")?0x6000:!strcmp(p,"ldr")?0x6800:!strcmp(p,"strb")?0x7000:0x7800;
                            ins=op|((unsigned)(off>>(byteop?0:2))<<6)|(base<<3)|r;
                        }
                    } else if(!strcmp(p,"adr")) {
                        int target=label(comma);if(target<0){if(pass)err=1;target=(int)pos+4;}
                        int base=((int)pos+4)&~3, delta=target-base;
                        if(pass && ((delta&3)||delta<0||delta>1020))err=1;
                        ins=0xa000 | (r<<8) | ((delta>>2)&255);
                    } else {
                        if(!number(comma,&v)){err=1;break;}
                        if(v<0||v>255)err=1;
                        if(!strcmp(p,"movs"))ins=0x2000|(r<<8)|(v&255);
                        else if(!strcmp(p,"adds"))ins=0x3000|(r<<8)|(v&255);
                        else if(!strcmp(p,"subs"))ins=0x3800|(r<<8)|(v&255);
                        else if(!strcmp(p,"cmp"))ins=0x2800|(r<<8)|(v&255);
                        else err=1;
                    }
                }
                if(err)break;emit16(ins,pass);
            }
        }
        if(!next)break;line=next;
    }
    return err?-1:(int)pos;
}
static int write_all(int fd,const uint8_t *data,int count){while(count>0){int n=write(fd,data,(size_t)count);if(n<=0)return -1;data+=n;count-=n;}return 0;}
int main(int argc,char **argv) {
    if(argc!=2){puts("usage: as /tmp/source.s");return 1;}
    int fd=open(argv[1],O_RDONLY);if(fd<0){puts("as: cannot open source");return 1;}
    int n=read(fd,src,MAX_SRC);if(n<0){close(fd);return 1;}
    char more;int extra=read(fd,&more,1);close(fd);
    if(n<=0||extra>0){puts("as: source must fit 1024 bytes");return 1;}
    src[n]=0;int len=compile(0);
    if(len<2||compile(1)!=len||len>MAX_OUT){puts("as: invalid instruction, label or size");return 1;}
    uint8_t hdr[8]={'P','N','X','1',(uint8_t)len,(uint8_t)(len>>8),0,0};
    fd=open("/tmp/a.out",O_WRONLY|O_CREAT|O_TRUNC);
    if(fd<0){puts("as: cannot create a.out");return 1;}
    int ok=write_all(fd,hdr,8)==0 && write_all(fd,code,len)==0;
    close(fd);if(!ok){puts("as: cannot write a.out");return 1;}
    printf("as: %d bytes (%d total) -> /tmp/a.out\n",len,len+8);return 0;
}
