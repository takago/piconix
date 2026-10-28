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
    puts("  a DUTY    Set GP18 duty cycle (0..100%)");
    puts("  b DUTY    Set GP19 duty cycle (0..100%)");
    puts("  off        Set both duty cycles to 0%");
}
static int eq(const char *a,const char *b) { return strcmp(a,b)==0; }
static int read_value(const char *path,unsigned *value) {
    int fd=open(path,O_RDONLY);
    if(fd<0)return -1;
    char b[24]; int n=read(fd,b,sizeof b-1);close(fd);
    if(n<=0)return -1;
    b[n]=0;
    unsigned x=0;int i=0;
    while(i<n && b[i]>='0' && b[i]<='9') {
        x=x*10u+(unsigned)(b[i]-'0');i++;
    }
    if(!i || (i<n && b[i]!='\n'))return -1;
    *value=x;return 0;
}
static int set_value(const char *path,unsigned value) {
    int fd=open(path,O_WRONLY);
    if(fd<0)return -1;
    char b[20];int n=snprintf(b,sizeof b,"%u\n",value);
    int written=write(fd,b,(size_t)n);
    close(fd);
    return written==n?0:-1;
}
static int parse_value(const char *s,unsigned limit,unsigned *out) {
    if(!*s)return -1;
    unsigned x=0;
    for(;*s;s++) {
        if(*s<'0' || *s>'9')return -1;
        unsigned digit=(unsigned)(*s-'0');
        if(x>(limit-digit)/10u)return -1;
        x=x*10u+digit;
    }
    *out=x;return 0;
}
static int status(void) {
    unsigned freq,a,b;
    if(read_value("/dev/pwm/1/freq",&freq)<0) return -1;
    printf("Frequency: %u Hz\n",freq);
    if(read_value("/dev/pwm/1/a",&a)==0)printf("Channel A (GP18): %u%%\n",a);
    else puts("Channel A (GP18): unavailable");
    if(read_value("/dev/pwm/1/b",&b)==0)printf("Channel B (GP19): %u%%\n",b);
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
        int a=set_value("/dev/pwm/1/a",0);
        int b=set_value("/dev/pwm/1/b",0);
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
    unsigned value;
    if(parse_value(argv[2],limit,&value)<0 || value<min) {
        puts("pwmctl: value out of range");return 1;
    }
    if(set_value(path,value)<0){puts("pwmctl: cannot write PWM device");return 1;}
    return 0;
}
