/* PicoNIX minimal netcat: IPv4 only, one socket per protocol. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include <stdint.h>

static int port(const char *s) {
    unsigned n=0;
    if(!*s)return -1;
    while(*s) {if(*s<'0'||*s>'9')return -1;n=n*10u+(unsigned)(*s++-'0');if(n>65535)return -1;}
    return n? (int)n:-1;
}
/* lwIP ip4_addr_t.addr is a network-order value in host uint32 storage. */
static int ipv4(const char *s,uint32_t *out) {
    unsigned a[4]={0},i=0,d=0;
    for(;*s;s++) {
        if(*s>='0'&&*s<='9') {if(++d>3)return -1;a[i]=a[i]*10+(*s-'0');if(a[i]>255)return -1;}
        else if(*s=='.'&&i<3&&d) {i++;d=0;}
        else return -1;
    }
    if(i!=3||!d)return -1;
    unsigned char *p=(unsigned char *)out;
    for(i=0;i<4;i++)p[i]=(unsigned char)a[i];
    return 0;
}
static int send_all(int fd,const void *buf,unsigned n) {
    const unsigned char *p=buf;
    while(n) {int r=send(fd,p,n,0);if(r==OS_EAGAIN||r==0){msleep(1);continue;}if(r<0)return -1;p+=r;n-=(unsigned)r;}
    return 0;
}
static void usage(void) {
    puts("usage: nc [-u] -l PORT | nc [-u] IPv4 PORT");
    puts("Ctrl+C / shell interrupt to exit; IPv4 numeric addresses only");
}
int main(int argc,char **argv) {
    unsigned char data[512];
    int udp=0,listening=0,i=1,fd=-1,peer=-1,number;
    if(i<argc&&!strcmp(argv[i],"-u")){udp=1;i++;}
    if(i<argc&&!strcmp(argv[i],"-l")){listening=1;i++;}
    if((listening&&argc-i!=1)||(!listening&&argc-i!=2)){usage();return 1;}
    uint32_t ip=0;
    if(!listening&&ipv4(argv[i++],&ip)<0){usage();return 1;}
    number=port(argv[i]);if(number<0){usage();return 1;}
    fd=socket(AF_INET,udp?SOCK_DGRAM:SOCK_STREAM,0);
    if(fd<0){puts("nc: socket unavailable");return 1;}
    if(udp) {
        struct sockaddr_in remote={0};
        remote.sin_family=AF_INET;
        remote.sin_port=htons((uint16_t)number);
        remote.sin_addr.s_addr=ip;
        if(listening) {
            struct sockaddr_in local={0};
            local.sin_family=AF_INET;
            local.sin_port=htons((uint16_t)number);
            if(bind(fd,(const struct sockaddr *)&local,sizeof(local))<0){puts("nc: bind failed");goto fail;}
        }
        /* stdin is nonblocking so incoming datagrams remain responsive. */
        fcntl(0,F_SETFL,O_NONBLOCK);
        for(;;) {
            struct sockaddr_in sender={0};
            socklen_t sender_len=sizeof(sender);
            int r=recvfrom(fd,data,sizeof(data),0,(struct sockaddr *)&sender,&sender_len);
            if(r>0) {remote=sender;if(send_all(1,data,(unsigned)r)<0)break;}
            else if(r!=OS_EAGAIN&&r<0)break;
            r=read(0,data,sizeof(data));
            if(r>0&&remote.sin_port) {
                if(sendto(fd,data,(size_t)r,0,(const struct sockaddr *)&remote,sizeof(remote))<0)break;
            } else if(r<0&&r!=OS_EAGAIN)break;
            msleep(2);
        }
    } else {
        if(listening) {
            struct sockaddr_in local={0};
            local.sin_family=AF_INET;
            local.sin_port=htons((uint16_t)number);
            if(bind(fd,(const struct sockaddr *)&local,sizeof(local))<0 || listen(fd,1)<0){puts("nc: listening failed");goto fail;}
            while((peer=accept(fd,0,0))==OS_EAGAIN)msleep(5);
            if(peer<0){puts("nc: accept failed");goto fail;}
        } else {
            struct sockaddr_in remote={0};
            remote.sin_family=AF_INET;
            remote.sin_port=htons((uint16_t)number);
            remote.sin_addr.s_addr=ip;
            if(connect(fd,(const struct sockaddr *)&remote,sizeof(remote))<0){puts("nc: connect failed");goto fail;}
            while((number=tcp_status(fd))==0)msleep(5);
            if(number!=1){puts("nc: connection failed");goto fail;}
            peer=fd;
        }
        fcntl(0,F_SETFL,O_NONBLOCK);
        for(;;) {
            int r=recv(peer,data,sizeof(data),0);
            if(r>0) {if(send_all(1,data,(unsigned)r)<0)break;}
            else if(r==0)break;
            else if(r!=OS_EAGAIN)break;
            r=read(0,data,sizeof(data));
            if(r>0){if(send_all(peer,data,(unsigned)r)<0)break;}
            else if(r<0&&r!=OS_EAGAIN)break;
            msleep(2);
        }
    }
    if(peer>=0&&peer!=fd)close(peer);
    close(fd);return 0;
fail:
    if(peer>=0&&peer!=fd)close(peer);
    close(fd);return 1;
}
