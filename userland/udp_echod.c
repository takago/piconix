/* UDP datagram echo server. */
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
    if(p<0){puts("usage: udp_echod [PORT]");return 1;}
    int fd=socket(AF_INET,SOCK_DGRAM,0);
    if(fd<0)return 1;
    struct sockaddr_in a={0};a.sin_family=AF_INET;a.sin_port=htons((uint16_t)p);
    if(bind(fd,(struct sockaddr *)&a,sizeof(a))<0){puts("udp_echod: bind failed");close(fd);return 1;}
    puts("udp_echod: listening");
    for(;;) {
        char buf[256];struct sockaddr_in peer={0};socklen_t len=sizeof(peer);
        int n=recvfrom(fd,buf,sizeof(buf),0,(struct sockaddr *)&peer,&len);
        if(n==OS_EAGAIN){msleep(2);continue;}
        if(n<0)break;
        /* One received datagram is returned as one datagram. */
        while(sendto(fd,buf,(size_t)n,0,(struct sockaddr *)&peer,len)==OS_EAGAIN)msleep(2);
    }
    close(fd);return 0;
}
