/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
int main(int argc,char *argv[]) {
    if(argc!=2) {puts("usage: kill PID");return 1;}
    char *end;long pid=strtol(argv[1],&end,10);
    if(*end || pid<=0 || pid>2147483647L || pid==getpid()) {puts("usage: kill PID (not self)");return 1;}
    if(kill((int)pid)<0) {puts("kill: no such task");return 1;}
    puts("terminated");return 0;
}
