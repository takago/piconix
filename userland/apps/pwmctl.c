/* PWM control utility for the existing /dev/pwm devices. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

static void usage(void) {
    puts("Usage: pwmctl [status | freq HZ | a DUTY | b DUTY | off]");
    puts("  status     Show PWM frequency and duty cycles");
    puts("  freq HZ    Set common frequency (8..100000 Hz)");
    puts("  a DUTY    Set GP18 duty cycle (0..100%, 0.1% steps)");
    puts("  b DUTY    Set GP19 duty cycle (0..100%, 0.1% steps)");
    puts("  off        Set both duty cycles to 0%");
}
static int eq(const char *a,const char *b) { return strcmp(a,b)==0; }
static int read_value(const char *path,char *value,size_t size) {
    int fd=open(path,O_RDONLY);
    if(fd<0)return -1;
    int n=read(fd,value,size-1);close(fd);
    if(n<=0)return -1;
    value[n]=0;
    if(value[n-1]=='\n')value[n-1]=0;
    return 0;
}
static int set_value(const char *path,const char *value) {
    int fd=open(path,O_WRONLY);
    if(fd<0)return -1;
    int n=(int)strlen(value);
    int written=write(fd,value,(size_t)n);
    close(fd);
    return written==n?0:-1;
}
static int valid_value(const char *s,unsigned limit,int duty) {
    unsigned x=0;int digits=0;
    while(*s>='0' && *s<='9') {
        unsigned d=(unsigned)(*s++-'0');
        if(x>100000u)return 0;
        x=x*10u+d;digits++;
    }
    if(!digits || x>limit)return 0;
    if(duty && *s=='.') {
        s++;
        if(*s<'0'||*s>'9')return 0;
        unsigned fraction=(unsigned)(*s++-'0');
        if(x==100 && fraction)return 0;
    }
    return *s==0;
}
static int status(void) {
    char freq[24],a[24],b[24];
    if(read_value("/dev/pwm/1/freq",freq,sizeof freq)<0)return -1;
    printf("Frequency: %s Hz\n",freq);
    if(read_value("/dev/pwm/1/a",a,sizeof a)==0)printf("Channel A (GP18): %s%%\n",a);
    else puts("Channel A (GP18): unavailable");
    if(read_value("/dev/pwm/1/b",b,sizeof b)==0)printf("Channel B (GP19): %s%%\n",b);
    else puts("Channel B (GP19): unavailable");
    return 0;
}
int main(int argc,char **argv) {
    if(argc==1 || (argc==2 && (eq(argv[1],"help") || eq(argv[1],"-h") || eq(argv[1],"--help")))) {
        usage();return 0;
    }
    if(argc==2 && eq(argv[1],"status")) {
        if(status()<0){puts("pwmctl: cannot read PWM status");return 1;}
        return 0;
    }
    if(argc==2 && eq(argv[1],"off")) {
        int a=set_value("/dev/pwm/1/a","0");
        int b=set_value("/dev/pwm/1/b","0");
        if(a<0 || b<0){puts("pwmctl: cannot disable one or more channels");return 1;}
        return 0;
    }
    if(argc!=3){usage();return 1;}
    const char *path;
    unsigned limit,min=0;
    if(eq(argv[1],"freq")){path="/dev/pwm/1/freq";min=8;limit=100000;}
    else if(eq(argv[1],"a")){path="/dev/pwm/1/a";limit=100;}
    else if(eq(argv[1],"b")){path="/dev/pwm/1/b";limit=100;}
    else {usage();return 1;}
    if(!valid_value(argv[2],limit,min==0) || (min && argv[2][0]=='0')) {
        puts("pwmctl: value out of range");return 1;
    }
    if(set_value(path,argv[2])<0){puts("pwmctl: cannot write PWM device");return 1;}
    return 0;
}
