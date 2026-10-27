/* SPDX-License-Identifier: GPL-3.0-or-later */
/* cat: read a file, proc node or byte stream to standard output. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"

static int output(const char *buf, int n) {
    int done=0;
    while(done<n) {
        int k=write(1,buf+done,(unsigned)(n-done));
        if(k<0) return -1;
        if(!k) {sleep(1);continue;}
        done+=k;
    }
    return 0;
}
int main(int argc, char *argv[]) {
    if(argc>2) {puts("usage: cat [PATH]");return 1;}
    const char *path=argc>1?argv[1]:0;
    if(path && !strcmp(path,"/dev/console")) {
        puts("cat: console reserved for shell");return 1;
    }
    int uart=path && !strcmp(path,"/dev/uart1");
    int fd=path?open(path,uart?O_NONBLOCK:O_RDONLY):0;
    if(fd<0) {puts("cat: cannot open");return 1;}
    char buf[32]; int n=0, rc=0;
    if(uart) {
        /* UART byte streams have no EOF: stop after 200 ms inactivity. */
        unsigned idle=0;
        while(idle<200) {
            n=read(fd,buf,sizeof buf);
            if(n<0) {rc=1;break;}
            if(n>0) {if(output(buf,n)<0) {rc=1;break;}idle=0;}
            else {sleep(10);idle+=10;}
        }
    } else {
        while((n=read(fd,buf,sizeof buf))>0)
            if(output(buf,n)<0) {rc=1;break;}
        if(n<0) rc=1;
    }
    if(path) close(fd);
    if(rc) puts("cat: read/write error");
    return rc;
}
