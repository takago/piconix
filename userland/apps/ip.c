/* PicoNIX interface summary. This is not Linux iproute2. */
#include "stdio.h"
#include "string.h"
#include "fcntl.h"
#include "unistd.h"

static void field(char *out, unsigned cap, const char *text, const char *key) {
    unsigned k=(unsigned)strlen(key);
    const char *p=text;
    while (*p) {
        if (!strncmp(p,key,k)) {
            p+=k; unsigned i=0;
            while (*p && *p!='\n' && i+1<cap) out[i++]=*p++;
            out[i]=0; return;
        }
        while (*p && *p!='\n') p++;
        if (*p) p++;
    }
    out[0]=0;
}
int main(int argc, char **argv) {
    if (argc>2 || (argc==2 && strcmp(argv[1],"addr") && strcmp(argv[1],"-h"))) {
        puts("usage: ip [addr]"); return 1;
    }
    if (argc==2 && !strcmp(argv[1],"-h")) {
        puts("usage: ip [addr]  (read-only interface summary)"); return 0;
    }
    char buf[640]; int fd=open("/proc/net/wifi",O_RDONLY);
    if(fd<0) { puts("ip: network status unavailable"); return 1; }
    int n=read(fd,buf,sizeof(buf)-1); close(fd);
    if(n<=0) { puts("ip: cannot read network status"); return 1; }
    buf[n]=0;
    char mode[16],name[16],ip[24],mask[24],mac[24],status[24];
    field(mode,sizeof mode,buf,"MODE: ");
    if(!strcmp(mode,"N/A")) { puts("ip: Wi-Fi interfaces require Pico W"); return 0; }
    field(name,sizeof name,buf,"IFACE: ");
    field(ip,sizeof ip,buf,"IP: ");
    field(mask,sizeof mask,buf,"NETMASK: ");
    field(mac,sizeof mac,buf,"MAC: ");
    field(status,sizeof status,buf,"STATUS: ");
    puts("lo0: 127.0.0.1/8 (loopback)");
    printf("%s: %s (%s)\n",name[0]?name:"wifi",mode,status[0]?status:"see net");
    printf("  inet %s  netmask %s\n  ether %s\n",ip,mask,mac);
    return 0;
}
