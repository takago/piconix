/* Single-client TCP echo server. */
#include "unistd.h"
#include "stdio.h"
#include <stdint.h>
static int port(const char *s) {
    unsigned n=0;
    if(!*s)return -1;
    while(*s) {if(*s<'0'||*s>'9')return -1;n=n*10u+(*s++-'0');if(n>65535)return -1;}
    return n?(int)n:-1;
}
int main(int argc,char **argv) {
    int p=argc==1?7:argc==2?port(argv[1]):-1;
    if(p<0){puts("usage: tcp_echod [PORT]");return 1;}
    int fd=socket(AF_INET,SOCK_STREAM,0);
    if(fd<0)return 1;
    struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons((uint16_t)p);
    if(bind(fd,(struct sockaddr *)&a,sizeof(a))<0||listen(fd,1)<0){puts("tcp_echod: bind failed");close(fd);return 1;}
    puts("tcp_echod: listening");
    for(;;) {
        int c=accept(fd,0,0);
        if(c==OS_EAGAIN){msleep(5);continue;}
        if(c<0)break;
        char buf[256];int n;
        while((n=recv(c,buf,sizeof(buf),0))!=0) {
            if(n==OS_EAGAIN){msleep(2);continue;}
            if(n<0)break;
            int off=0;
            while(off<n){int w=send(c,buf+off,(size_t)(n-off),0);
                if(w==OS_EAGAIN||w==0){msleep(2);continue;}
                if(w<0)break;
                off+=w;
            }
            if(off<n)break;
        }
        close(c);
    }
    close(fd);return 0;
}
