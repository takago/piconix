/* SPDX-License-Identifier: GPL-3.0-or-later */
/* List open descriptors using the existing /proc task snapshots. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"

static int number(const char **p) { int v=0; while(**p>='0'&&**p<='9') {v=v*10+**p-'0';++*p;} return v; }
static int show_task(int pid) {
    char path[32];
    snprintf(path,sizeof path,"/proc/task/%d",pid);
    int fd=open(path,O_RDONLY);
    if(fd<0) return 0; /* Task may have exited. */
    char name[32]="?";
    char data[1600]; unsigned used=0;
    int n;
    while(used<sizeof(data)-1 && (n=read(fd,data+used,sizeof(data)-1-used))>0)
        used+=(unsigned)n;
    close(fd);
    data[used]=0;
    for(char *p=data; *p;) {
        char *end=strchr(p,'\n');
        if(end) *end=0;
        if(!strncmp(p,"Name: ",6)) {
            unsigned i=0;
            while(p[6+i] && i<sizeof(name)-1) {name[i]=p[6+i];i++;}
            name[i]=0;
        } else if(!strncmp(p,"  fd ",5)) {
            const char *q=p+5; int f=number(&q);
            if(!strncmp(q,": device=",9)) {
                q+=9;
                (void)number(&q);
                if(!strncmp(q," flags=",7)) {
                    q+=7; int flags=number(&q);
                    const char *path=strstr(q," name=");
                    printf("%-5d %-12s %-3d %-5d %s\n",pid,name,f,flags,
                           path?path+6:"?");
                }
            }
        }
        if(!end) break;
        p=end+1;
    }
    return 0;
}
int main(int argc,char **argv) {
    if(argc>2) {puts("usage: lsof [pid]");return 1;}
    puts("PID   COMMAND      FD  FLAGS NAME");
    if(argc==2) {int pid=0;for(char *p=argv[1];*p;p++) {if(*p<'0'||*p>'9') return 1;pid=pid*10+*p-'0';}return show_task(pid);}
    int fd=open("/proc/tasks",O_RDONLY);
    if(fd<0) {puts("lsof: cannot open /proc/tasks");return 1;}
    char buf[1024]; unsigned used=0; int n;
    while(used<sizeof(buf)-1 && (n=read(fd,buf+used,sizeof(buf)-1-used))>0) used+=(unsigned)n;
    close(fd);buf[used]=0;
    for(char *p=buf;*p;) {
        char *end=strchr(p,'\n');if(end)*end=0;
        int pid=0; char *q=p;while(*q==' ')q++;
        if(*q>='0'&&*q<='9') {while(*q>='0'&&*q<='9')pid=pid*10+*q++-'0';show_task(pid);}
        if(!end)break;p=end+1;
    }
    return 0;
}
