/* SPDX-License-Identifier: GPL-3.0-or-later */
/* ps: task listing provided by the kernel's /proc/tasks. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    int fd=open("/proc/tasks",O_RDONLY);
    if(fd<0) { puts("ps: cannot open /proc/tasks"); return 1; }
    char buf[64]; int n;
    while((n=read(fd,buf,sizeof buf))>0) {
        int done=0;
        while(done<n) { int k=write(1,buf+done,(unsigned)(n-done));
            if(k<0) {close(fd);return 1;}
            if(!k) {msleep(1);continue;}
            done+=k;
        }
    }
    close(fd);
    return n<0 ? 1 : 0;
}
