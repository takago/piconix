/* Framebuffer control utility. */
#include <stdint.h>
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "../include/piconix.h"

static const uint8_t heart[8]={0x66,0xff,0xff,0xff,0x7e,0x3c,0x18,0};
static int same(const char *a,const char *b){return strcmp(a,b)==0;}
static int pattern(int fd,const uint8_t b[8]){return write(fd,b,8)==8?0:-1;}
/* Display the same kernel-generated report as cat /proc/fb. */
static int status(void){
    int fd=open("/proc/fb",O_RDONLY);
    if(fd<0){puts("fbctl: cannot open /proc/fb");return -1;}
    char buf[64];
    int n,rc=0;
    while((n=read(fd,buf,sizeof buf))>0){
        int done=0;
        while(done<n){
            int k=write(1,buf+done,(unsigned)(n-done));
            if(k<0){rc=-1;break;}
            if(k==0){msleep(1);continue;}
            done+=k;
        }
        if(rc<0)break;
    }
    if(n<0)rc=-1;
    close(fd);
    return rc;
}
static void usage(void){
    puts("Usage: fbctl <command>");
    puts("");
    puts("Commands:");
    puts("  on      Enable framebuffer");
    puts("  off     Disable framebuffer and release GPIO");
    puts("  clear   Clear framebuffer");
    puts("  status  Display /proc/fb");
    puts("  test    Blink heart three times");
}
int main(int argc,char **argv){
    if(argc==1){usage();return 0;}
    if(argc==2 && (same(argv[1],"help") || same(argv[1],"-h") || same(argv[1],"--help"))){usage();return 0;}
    if(argc==2 && same(argv[1],"status")){
        if(status()<0){puts("fbctl: status error");return 1;}
        return 0;
    }
    if(argc!=2 || (!same(argv[1],"on") && !same(argv[1],"off") &&
                   !same(argv[1],"clear") && !same(argv[1],"test"))){
        usage();return 1;
    }
    int fd=open("/dev/gpio/fb",O_RDWR);
    if(fd<0){puts("fbctl: cannot open framebuffer");return 1;}
    int rc=0;
    if(same(argv[1],"on")) rc=ioctl(fd,OS_FB_ENABLE,0);
    else if(same(argv[1],"off")) rc=ioctl(fd,OS_FB_DISABLE,0);
    else if(same(argv[1],"clear")){
        uint8_t empty[8]={0};rc=pattern(fd,empty);
    }else if(same(argv[1],"test")){
        rc=ioctl(fd,OS_FB_ENABLE,0);
        if(rc>=0)rc=pattern(fd,heart);
        /* Blink three times; leave the heart displayed. */
        uint8_t empty[8]={0};
        for(int i=0;rc>=0 && i<3;i++) {
            msleep(500);
            rc=pattern(fd,empty);
            if(rc<0)break;
            msleep(500);
            rc=pattern(fd,heart);
        }
    }else{usage();rc=-1;}
    close(fd);
    if(rc<0){puts("fbctl: operation failed");return 1;}
    return 0;
}
