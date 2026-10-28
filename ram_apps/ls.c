/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "../src/pins.h"
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
static void ls(const char *path) {
    if(eq(path,"/")) { put("bin/\r\ndev/\r\nproc/\r\ntmp/\r\n"); return; }
    if(eq(path,"/dev") || eq(path,"/dev/") ||
       eq(path,"/tmp") || eq(path,"/tmp/") ||
       eq(path,"/bin") || eq(path,"/bin/")) {
        const char *listing=eq(path,"/dev") || eq(path,"/dev/") ?
            "/proc/devices" : (eq(path,"/bin") || eq(path,"/bin/")) ? "/bin" : "/tmp";
        int fd=open(listing, 0);
        if(fd<0) { put("ls: cannot open directory\r\n"); return; }
        char b[32]; int n;
        while((n=read(fd, b, sizeof b))>0)
            write_all(1,b,(size_t)n);
        if(n<0) put("ls: read error\r\n");
        close(fd);
        if(eq(listing,"/bin")) put("\r\n---- sh built-in commands ----\r\nhelp\r\nlogout\r\nexit\r\ncd\r\ngetty\r\n");
    }
    else if(eq(path,"/dev/gpio") || eq(path,"/dev/gpio/")) {
        put("fb\r\n");
        for(int i=0;i<=28;i++) {
            if(!pin_gpio((unsigned)i)) continue;
            number(i); put("\r\n");
            number(i); put(".dir\r\n");
            number(i); put(".pull\r\n");
        }
    } else if(eq(path,"/dev/adc") || eq(path,"/dev/adc/"))
        { for(unsigned ch=0;ch<3;ch++) if(pin_adc(ch)) {number((int)ch);put("\r\n");} put("temp\r\n"); }
    else if(eq(path,"/dev/pwm") || eq(path,"/dev/pwm/"))
        { if(pin_any_pwm()) put("1/\r\n"); }
    else if(eq(path,"/dev/pwm/1") || eq(path,"/dev/pwm/1/"))
        { if(pin_any_pwm()) put("freq\r\n"); if(pin_pwm(18)) put("a\r\n"); if(pin_pwm(19)) put("b\r\n"); }
    else if(eq(path,"/proc") || eq(path,"/proc/"))
        put("uptime\r\nticks\r\ninterrupts\r\ncontext_switches\r\nmemory\r\ndevices\r\ntasks\r\ntask/\r\nfb\r\n");
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
                    write_all(1,b+start,j-start);put("\r\n");
                } else if(b[start]==' ') {
                    unsigned j=start;while(j<i && b[j]==' ') j++;
                    unsigned k=j;while(k<i && b[k]>='0' && b[k]<='9') k++;
                    if(k>j) {write_all(1,b+j,k-j);put("\r\n");}
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
    if(argc>2) {puts("usage: ls [PATH]");return 1;}
    if(!getcwd(cwd,sizeof cwd)) {puts("ls: cannot get cwd");return 1;}
    if(resolve(cwd,argc==2?argv[1]:".",path)<0) {puts("ls: path too long");return 1;}
    ls(path);
    return 0;
}
