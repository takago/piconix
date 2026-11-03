/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "stdio.h"
#include "string.h"
static int set(int cmd,int v) { return ioctl(0,cmd,v); }
int main(int argc,char **argv) {
    if(argc==1) {
        int canon=ioctl(0,OS_TTY_GET_CANON,0),echo=ioctl(0,OS_TTY_GET_ECHO,0),onlcr=ioctl(0,OS_TTY_GET_ONLCR,0);
        if(canon<0 || echo<0 || onlcr<0) goto error;
        printf("%s %s %s\n",canon?"icanon":"-icanon",echo?"echo":"-echo",onlcr?"onlcr":"-onlcr");
        return 0;
    }
    for(int i=1;i<argc;i++) {
        int cmd=-1,value=1;
        if(!strcmp(argv[i],"icanon")) cmd=OS_TTY_SET_CANON;
        else if(!strcmp(argv[i],"-icanon")) {cmd=OS_TTY_SET_CANON;value=0;}
        else if(!strcmp(argv[i],"onlcr")) cmd=OS_TTY_SET_ONLCR;
        else if(!strcmp(argv[i],"-onlcr")) {cmd=OS_TTY_SET_ONLCR;value=0;}
        else if(!strcmp(argv[i],"echo")) cmd=OS_TTY_SET_ECHO;
        else if(!strcmp(argv[i],"-echo")) {cmd=OS_TTY_SET_ECHO;value=0;}
        else if(!strcmp(argv[i],"raw")) {
            if(set(OS_TTY_SET_CANON,0)<0 || set(OS_TTY_SET_ECHO,0)<0) goto error;
            continue;
        } else if(!strcmp(argv[i],"cooked")) {
            if(set(OS_TTY_SET_CANON,1)<0 || set(OS_TTY_SET_ECHO,1)<0) goto error;
            continue;
        }
        if(cmd<0 || set(cmd,value)<0) goto error;
    }
    ioctl(0,OS_TTY_PERSIST,0);
    return 0;
error:
    puts("usage: stty [raw|cooked|icanon|-icanon|echo|-echo|onlcr|-onlcr] ...");
    return 1;
}
