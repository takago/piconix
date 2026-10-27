/* SPDX-License-Identifier: GPL-3.0-or-later */
/* watch: repeat an external program; no shell parser or new syscalls. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static int program_id(const char *name) {
    int fd=open("/bin",O_RDONLY);
    if(fd<0) return -1;
    char buf[64],line[32]; int id=1,used=0,found=-1,n;
    while((n=read(fd,buf,sizeof buf))>0) {
        for(int i=0;i<n;i++) {
            if(buf[i]=='\n') {
                line[used]=0;
                if(!strcmp(name,line)) {found=id;break;}
                id++;used=0;
            } else if(used<(int)sizeof line-1) line[used++]=buf[i];
        }
        if(found>0) break;
    }
    close(fd);
    return found;
}
int main(int argc, char *argv[]) {
    unsigned interval=1;
    int first=1;
    if(argc>1 && argv[1][0]>='0' && argv[1][0]<='9') {
        char *end;
        long n=strtol(argv[1],&end,10);
        if(*end || n<1 || n>60) {puts("watch: interval must be 1..60 seconds");return 1;}
        interval=(unsigned)n;
        first=2;
    }
    if(argc<=first) {puts("usage: watch [seconds] command [args...]");return 1;}
    const char *name=argv[first];
    if(!strncmp(name,"/bin/",5)) name+=5;
    int id=program_id(name);
    if(id<0 || !strcmp(name,"watch")) {puts("watch: unknown or recursive command");return 1;}
    const char *args[8]; int count=argc-first;
    if(count>8) {puts("watch: too many arguments");return 1;}
    for(int i=0;i<count;i++) args[i]=argv[first+i];
    args[0]=name;
    for(;;) {
        /* ANSI home + clear screen; keep the display in normal mode. */
        printf("\033[H\033[2JEvery %us: ",interval);
        for(int i=0;i<count;i++) printf("%s%s",i?" ":"",args[i]);
        puts("\n");
        int child=spawn(id,count,args);
        if(child<0) {puts("watch: cannot start command");return 1;}
        if(waitpid(child,0,0)<0) {puts("watch: waitpid failed");return 1;}
        sleep(interval*1000u);
    }
}
