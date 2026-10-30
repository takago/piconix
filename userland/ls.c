/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "../kernel/pins.h"
static int eq(const char *a,const char *b) {return strcmp(a,b)==0;}
static void write_all(int fd,const char *p,size_t n) {while(n) {int k=write(fd,p,n);if(k<0)return;if(!k){msleep(1);continue;}p+=k;n-=k;}}
static void put(const char *s) {write_all(1,s,strlen(s));}
static void number(int v) {char b[16];snprintf(b,sizeof b,"%d",v);put(b);}
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
static int detailed;
/* File sizes are available for writable /tmp files only; other nodes are virtual. */
static void entry(const char *directory,const char *name,int directory_entry) {
    if(detailed) {
        if(directory_entry)put("d    -  ");
        else if(eq(directory,"/tmp")) {
            char path[128];
            snprintf(path,sizeof path,"/tmp/%s",name);
            int fd=open(path,0);
            if(fd>=0) {
                char buf[64];int n;unsigned size=0;
                while((n=read(fd,buf,sizeof buf))>0)size+=(unsigned)n;
                close(fd);
                if(n>=0){put("- ");number((int)size);put("  ");}
                else put("-    -  ");
            } else put("-    -  ");
        } else put("-    -  ");
    }
    put(name);put("\r\n");
}
static void list_stream(const char *path) {
    int fd=open(path,0);
    if(fd<0){put("ls: cannot open directory\r\n");return;}
    char line[64];unsigned len=0;char c;
    while(read(fd,&c,1)==1) {
        if(c=='\n') {
            line[len]=0;
            if(len)entry(path,line,0);
            len=0;
        } else if(len<sizeof line-1)line[len++]=c;
    }
    if(len){line[len]=0;entry(path,line,0);}
    close(fd);
}
static void ls(const char *path) {
    if(eq(path,"/")) { entry(path,"bin/",1);entry(path,"dev/",1);entry(path,"proc/",1);entry(path,"tmp/",1);return; }
    if(eq(path,"/dev") || eq(path,"/dev/") ||
       eq(path,"/tmp") || eq(path,"/tmp/") ||
       eq(path,"/bin") || eq(path,"/bin/")) {
        const char *listing=eq(path,"/dev") || eq(path,"/dev/") ?
            "/proc/devices" : (eq(path,"/bin") || eq(path,"/bin/")) ? "/bin" : "/tmp";
        list_stream(listing);
        if(eq(listing,"/bin")) {put("\r\n---- sh built-in commands ----\r\n");entry("/bin","help",0);entry("/bin","logout",0);entry("/bin","exit",0);entry("/bin","cd",0);entry("/bin","getty",0);}
    }
    else if(eq(path,"/dev/gpio") || eq(path,"/dev/gpio/")) {
        entry(path,"fb",0);
        for(int i=0;i<=28;i++) {
            if(!pin_gpio((unsigned)i)) continue;
            {char name[16];snprintf(name,sizeof name,"%d",i);entry(path,name,0);}
            {char name[16];snprintf(name,sizeof name,"%d.dir",i);entry(path,name,0);}
            {char name[16];snprintf(name,sizeof name,"%d.pull",i);entry(path,name,0);}
        }
    } else if(eq(path,"/dev/adc") || eq(path,"/dev/adc/"))
        { for(unsigned ch=0;ch<3;ch++) if(pin_adc(ch)) {char name[8];snprintf(name,sizeof name,"%d",(int)ch);entry(path,name,0);} entry(path,"temp",0); }
    else if(eq(path,"/dev/pwm") || eq(path,"/dev/pwm/"))
        { if(pin_any_pwm()) entry(path,"1/",1); }
    else if(eq(path,"/dev/pwm/1") || eq(path,"/dev/pwm/1/"))
        { if(pin_any_pwm()) entry(path,"freq",0); if(pin_pwm(18)) entry(path,"a",0); if(pin_pwm(19)) entry(path,"b",0); }
    else if(eq(path,"/proc") || eq(path,"/proc/"))
        {entry(path,"uptime",0);entry(path,"ticks",0);entry(path,"interrupts",0);
         entry(path,"context_switches",0);entry(path,"memory",0);entry(path,"ramfiles",0);entry(path,"devices",0);
         entry(path,"tasks",0);entry(path,"task/",1);entry(path,"fb",0);entry(path,"sym/",1);}
    else if(eq(path,"/proc/sym") || eq(path,"/proc/sym/"))
        {entry(path,"kernel",0);entry(path,"userland/",1);}
    else if(eq(path,"/proc/sym/userland") || eq(path,"/proc/sym/userland/"))
        list_stream("/bin");
    else if(eq(path,"/proc/task") || eq(path,"/proc/task/")) {
        /* /proc/tasks is the authoritative live-task list. */
        int fd=open("/proc/tasks",0);
        if(fd<0) {put("ls: cannot open /proc/tasks\r\n");return;}
        char b[128]; int n; unsigned used=0;
        while((n=read(fd,b+used,sizeof b-used-1))>0) {
            used+=(unsigned)n; b[used]=0;
            unsigned start=0;
            for(unsigned i=0;i<used;i++) if(b[i]=='\n') {
                if(b[start]>='0' && b[start]<='9') {
                    unsigned j=start; while(j<i && b[j]>='0' && b[j]<='9') j++;
                    {char name[16];unsigned len=j-start;if(len>15)len=15;memcpy(name,b+start,len);name[len]=0;entry(path,name,1);}
                } else if(b[start]==' ') {
                    unsigned j=start;while(j<i && b[j]==' ') j++;
                    unsigned k=j;while(k<i && b[k]>='0' && b[k]<='9') k++;
                    if(k>j) {{char name[16];unsigned len=k-j;if(len>15)len=15;memcpy(name,b+j,len);name[len]=0;entry(path,name,1);}}
                }
                start=i+1;
            }
            if(start) {used-=start;memmove(b,b+start,used);}
            if(used>=sizeof b-1) break;
        }
        close(fd);
    }
    else put("ls: supported: /bin /dev /dev/gpio /proc /tmp\r\n");
}
int main(int argc,char *argv[]) {
    char cwd[128], path[128];
    const char *arg=".";
    if(argc>=2 && eq(argv[1],"-l")) {detailed=1;if(argc==3)arg=argv[2];else if(argc>3){puts("usage: ls [-l] [PATH]");return 1;}}
    else if(argc==2)arg=argv[1];
    else if(argc>2){puts("usage: ls [-l] [PATH]");return 1;}
    if(!getcwd(cwd,sizeof cwd)) {puts("ls: cannot get cwd");return 1;}
    if(resolve(cwd,arg,path)<0) {puts("ls: path too long");return 1;}
    ls(path);
    return 0;
}
