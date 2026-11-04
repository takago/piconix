/* SPDX-License-Identifier: GPL-3.0-or-later */
/* 観測用デバイスからパケット記録を読み出して表示する． */

/* ペイロード全体は保存せず，IPv4ヘッダと先頭16バイトを観測する． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "../../include/piconix_net_capture.h"
/* IPv4アドレス4バイトをドット区切りで表示する． */
static void ip(const uint8_t *p) { printf("%u.%u.%u.%u",p[0],p[1],p[2],p[3]); }

/* IPv4プロトコル番号をTCP／UDP／ICMP等の表示名に対応付ける． */
static const char *protocol(unsigned n) {
    return n==6 ? "TCP" : n==17 ? "UDP" : n==1 ? "ICMP" : "IP";
}

/* 保存されたパケット要約を読める形式で出力する．-x時は先頭データも表示する． */
static void show(const piconix_packet_t *p,int hex) {
    printf("%s %s ",p->direction ? "TX" : "RX",protocol(p->protocol));
    ip(p->src);
    if(p->protocol==6 || p->protocol==17) printf(":%u",p->sport);
    printf(" > "); ip(p->dst);
    if(p->protocol==6 || p->protocol==17) printf(":%u",p->dport);
    printf(" LEN=%u",p->ip_len);
    if(hex) printf(" DATA=%u",p->data_len);
    if(p->protocol==6)
        printf("%s%s%s%s%s",(p->flags&2)?" SYN":"",(p->flags&16)?" ACK":"",
               (p->flags&1)?" FIN":"",(p->flags&4)?" RST":"",(p->flags&8)?" PSH":"");
    if(p->protocol==1) printf(" TYPE=%u",p->flags);
    putchar('\n');
    if(hex && p->sample_len) {
        printf("    DATA:  ");
        for(unsigned i=0;i<16;i++) {
            if(i<p->sample_len) printf("%02x ",p->sample[i]);
            else printf("   ");
        }
        printf("| ");
        for(unsigned i=0;i<p->sample_len;i++) {
            unsigned c=p->sample[i]; putchar(c>=32&&c<127?(int)c:'.');
        }
        puts("|");
    }
}

/* /dev/net/packetから受信・送信パケットの要約を読み，指定件数まで表示する． */
int main(int argc,char **argv) {
    int hex=0,limit=0;
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"-x")) hex=1;
        else if(!strcmp(argv[i],"-c") && ++i<argc) {
            for(const char *p=argv[i];*p;p++) {
                if(*p<'0'||*p>'9'||limit>1000) {limit=0;break;}
                limit=limit*10+*p-'0';
            }
            if(!limit) {puts("usage: tcpdump [-x] [-c COUNT]");return 1;}
        } else {puts("usage: tcpdump [-x] [-c COUNT]");return 1;}
    }
    int fd=open("/dev/net/packet",O_RDONLY);
    if(fd<0) {puts("tcpdump: packet capture requires Pico W");return 1;}
    puts("tcpdump: IPv4 capture (Ctrl+C to stop)");
    for(int count=0;!limit||count<limit;) {
        piconix_packet_t p;
        int n=read(fd,&p,sizeof(p));
        if(n==OS_EAGAIN) {msleep(10);continue;}
        if(n!=(int)sizeof(p)) {puts("tcpdump: read error");close(fd);return 1;}
        show(&p,hex);count++;
    }
    close(fd);return 0;
}
