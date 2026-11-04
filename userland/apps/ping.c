/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Four ICMP Echo probes by default; no raw socket syscall required. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "../../include/piconix_net_capture.h"
static int ipv4(const char *text,uint8_t out[4]) {
    for(unsigned i=0;i<4;i++) {
        unsigned v=0,digits=0;
        while(*text>='0'&&*text<='9') {
            v=v*10u+(unsigned)(*text++-'0');
            if(v>255||++digits>3)return -1;
        }
        if(!digits || (i<3 ? *text++!='.' : *text!='\0'))return -1;
        out[i]=(uint8_t)v;
    }
    return 0;
}
int main(int argc,char **argv) {
    int count=4, i=1;
    if(argc>2&&!strcmp(argv[1],"-c")) {
        count=0;
        for(const char *p=argv[2];*p;p++) {
            if(*p<'0'||*p>'9'||count>100) {count=0;break;}
            count=count*10+*p-'0';
        }
        i=3;
    }
    uint8_t address[4];
    if(argc!=i+1||count<1||count>100||ipv4(argv[i],address)<0) {
        puts("usage: ping [-c COUNT] IPv4   (default: 4 probes)");return 1;
    }
    int fd=open("/dev/net/icmp",O_RDWR);
    if(fd<0) {puts("ping: ICMP device unavailable or busy (Pico W only)");return 1;}
    int received=0;
    for(int seq=1;seq<=count;seq++) {
        piconix_echo_t request={0};
        memcpy(request.address,address,4);
        request.id=(uint16_t)getpid();request.sequence=(uint16_t)seq;
        if(write(fd,&request,sizeof(request))!=(int)sizeof(request)) {
            puts("ping: send failed");break;
        }
        int answered=0;
        /* Kernel's poll loop runs while this task sleeps. */
        for(int elapsed=0;elapsed<1250;elapsed+=10) {
            piconix_echo_t reply;
            int n=read(fd,&reply,sizeof(reply));
            if(n==(int)sizeof(reply) && reply.sequence==request.sequence) {
                if(reply.milliseconds!=0xffffffffu) {
                    printf("reply from %u.%u.%u.%u: seq=%u time=%u ms\n",
                        address[0],address[1],address[2],address[3],seq,(unsigned)reply.milliseconds);
                    received++;
                } else printf("timeout: seq=%u\n",seq);
                answered=1;break;
            }
            if(n!=OS_EAGAIN) break;
            msleep(10);
        }
        if(!answered) printf("timeout: seq=%u\n",seq);
        if(seq<count) msleep(1000);
    }
    printf("%d transmitted, %d received\n",count,received);
    close(fd);return received==count?0:1;
}
