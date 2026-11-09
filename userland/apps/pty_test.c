/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Interactive PTY relay: console <-> master <-> slave <-> child.
 * With no arguments, run tty as a basic PTY smoke test. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stddef.h>

#define RELAY_BYTES 128

/* 物理コンソールへの部分書き込みを再試行し，全バイトを転送する． */
static int output_all(int fd, const char *buf, int len) {
    int sent=0, stalls=0;
    while(sent<len) {
        int n=write(fd,buf+sent,(size_t)(len-sent));
        if(n>0) { sent+=n; stalls=0; continue; }
        if(n==0 || n==OS_EAGAIN) {
            if(++stalls>5000) return -1;
            msleep(1); continue;
        }
        return -1;
    }
    return 0;
}

/* 指定されたプログラムをPTYのSlaveで起動し，Masterと実コンソールを中継する． */
int main(int argc, char **argv) {
    const char *test_args[]={"tty"};
    const char *const *child_args=argc>1 ? (const char *const *)(argv+1) : test_args;
    int child_argc=argc>1 ? argc-1 : 1;
    int program=program_id(child_args[0]);
    if(program<0) { printf("pty_test: command not found: %s\n",child_args[0]); return 1; }

    int master=-1,slave=-1,saved[3]={-1,-1,-1},child=-1;
    int old_flags=-1,old_canon=-1,old_echo=-1,old_intr=-1;
    int rc=0, child_done=0, saw_eof=0;
    char input[RELAY_BYTES],output[RELAY_BYTES];
    int in_pos=0,in_len=0;

    master=open("/dev/ptmx",O_RDWR|O_NONBLOCK);
    if(master<0) { printf("pty_test: cannot open /dev/ptmx (%d)\n",master); return 1; }
    slave=open("/dev/pts/0",O_RDWR);
    if(slave<0) { printf("pty_test: cannot open slave (%d)\n",slave); rc=1; goto cleanup; }
    for(int i=0;i<3;i++) saved[i]=dup(i);
    if(saved[0]<0 || saved[1]<0 || saved[2]<0) {
        puts("pty_test: cannot save standard descriptors"); rc=1; goto cleanup;
    }
    for(int i=0;i<3;i++) if(dup2(slave,i)<0) {
        /* Restore all original descriptors below. */
        puts("pty_test: dup2 failed"); rc=1; goto restore_fds;
    }
    child=spawn_foreground(program,child_argc,child_args);
restore_fds:
    for(int i=0;i<3;i++) if(saved[i]>=0) {
        (void)dup2(saved[i],i);
        close(saved[i]); saved[i]=-1;
    }
    close(slave); slave=-1;
    if(child<0) {
        if(!rc) printf("pty_test: spawn failed (%d)\n",child);
        rc=1; goto cleanup;
    }

    /* Preserve the real console's mode and restore it before returning to sh. */
    old_canon=ioctl(0,OS_TTY_GET_CANON,0);
    old_echo=ioctl(0,OS_TTY_GET_ECHO,0);
    old_intr=ioctl(0,OS_TTY_GET_INTR,0);
    old_flags=fcntl(0,F_GETFL,0);
    if(old_canon<0 || old_echo<0 || old_intr<0 || old_flags<0 ||
       ioctl(0,OS_TTY_SET_INTR,0)<0 ||
       ioctl(0,OS_TTY_SET_CANON,0)<0 ||
       ioctl(0,OS_TTY_SET_ECHO,0)<0 ||
       fcntl(0,F_SETFL,old_flags|O_NONBLOCK)<0) {
        puts("pty_test: console is not an interactive TTY");
        rc=1; goto cleanup;
    }

    /* Keep reading the PTY while forwarding keys to it; never discard partial
     * writes when the PTY input queue is full. */
    for(;;) {
        fd_set rd=0,wr=0;
        FD_SET(master,&rd);
        if(!child_done && in_len==0) FD_SET(0,&rd);
        if(in_len>in_pos) FD_SET(master,&wr);
        struct timeval tv={0,100000};
        int nfds=(master>0 ? master : 0)+1;
        int ready=select(nfds,&rd,&wr,NULL,&tv);
        if(ready<0) { rc=1; break; }

        if(FD_ISSET(master,&rd)) {
            int n=read(master,output,sizeof output);
            if(n>0) { if(output_all(1,output,n)<0) { rc=1; break; } }
            else if(n==0) { saw_eof=1; break; }
            else if(n!=OS_EAGAIN) { rc=1; break; }
        }
        if(!child_done && in_len==0 && FD_ISSET(0,&rd)) {
            int n=read(0,input,sizeof input);
            if(n>0) { in_len=n; in_pos=0; }
            else if(n<0 && n!=OS_EAGAIN) { rc=1; break; }
        }
        if(in_len>in_pos && FD_ISSET(master,&wr)) {
            int n=write(master,input+in_pos,(size_t)(in_len-in_pos));
            if(n>0) {
                in_pos+=n;
                if(in_pos==in_len) in_len=in_pos=0;
            } else if(n<0 && n!=OS_EAGAIN) { rc=1; break; }
        }
        if(!child_done) {
            int r=waitpid(child,NULL,WNOHANG);
            if(r==child) child_done=1;
            else if(r<0) { rc=1; break; }
        }
    }

cleanup:
    /* An interrupted relay must not leave a child holding the PTY forever. */
    if(child>0 && !child_done) {
        int r=waitpid(child,NULL,WNOHANG);
        if(r!=child) {
            if(r==0) { kill(child); (void)waitpid(child,NULL,0); }
            rc=1;
        }
    }
    if(old_flags>=0) (void)fcntl(0,F_SETFL,old_flags);
    if(old_echo>=0) (void)ioctl(0,OS_TTY_SET_ECHO,old_echo);
    if(old_intr>=0) (void)ioctl(0,OS_TTY_SET_INTR,old_intr);
    if(old_canon>=0) (void)ioctl(0,OS_TTY_SET_CANON,old_canon);
    if(slave>=0) close(slave);
    for(int i=0;i<3;i++) if(saved[i]>=0) {
        (void)dup2(saved[i],i); close(saved[i]);
    }
    if(master>=0) close(master);
    if(!saw_eof && child>0) rc=1;
    return rc;
}
