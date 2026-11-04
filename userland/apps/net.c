/* ネットワーク接続の診断情報を表示する． */
/* ネットワーク診断値をユーザ空間で整形して表示する． */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
/* ネットワーク診断用の/procファイルを開き，内容を出力する． */
static int show(const char *path) {
    int fd=open(path,O_RDONLY);
    if(fd<0) {puts("net: cannot open network information");return 1;}
    char buf[96];int n,rc=0;
    while((n=read(fd,buf,sizeof(buf)))>0) {
        int done=0;
        while(done<n) {
            int k=write(1,buf+done,(unsigned)(n-done));
            if(k<0){rc=1;break;}
            if(!k){msleep(1);continue;}
            done+=k;
        }
        if(rc)break;
    }
    if(n<0)rc=1;
    close(fd);
    return rc;
}

/* Wi-Fi・ARP等のネットワーク状態と診断情報を表示する． */
int main(int argc,char **argv) {
    if(argc==1)return show("/proc/net/wifi");
    if(argc==2) {
        if(!strcmp(argv[1],"-a"))return show("/proc/net/ap");
        if(!strcmp(argv[1],"-r"))return show("/proc/net/arp");
        if(!strcmp(argv[1],"-h")||!strcmp(argv[1],"--help")){
            puts("Usage: net [-a|-r]");
            puts("  net     Wi-Fi, IP and DHCP status");
            puts("  net -a  AP associated clients (MAC addresses)");
            puts("  net -r  ARP cache (IP and MAC addresses)");
            return 0;
        }
    }
    puts("Usage: net [-a|-r]");return 1;
}
