/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "../include/piconix_i2c.h"
#include <stdint.h>
static int number(const char *s,int *v) {
    char *end; long n=strtol(s,&end,0);
    if(!*s || *end || n<0 || n>255) return -1;
    *v=(int)n; return 0;
}
static void usage(void) {
    puts("Usage: i2cctl scan | read ADDR COUNT | write ADDR BYTE... | xfer ADDR READ_COUNT [BYTE...]");
    puts("Addresses and bytes accept decimal or 0x-prefixed hex; max 32 bytes.");
}
int main(int argc,char **argv) {
    if(argc<2) {usage();return 1;}
    int fd=open("/dev/i2c/1",O_RDWR);
    if(fd<0) {puts("i2cctl: cannot open /dev/i2c/1");return 1;}
    piconix_i2c_transfer_t t={0};
    if(!strcmp(argv[1],"scan") && argc==2) {
        puts("I2C1 scan (GP26 SDA, GP27 SCL):");
        int found=0;
        for(int a=0x08;a<=0x77;a++) {
            t.address=(uint8_t)a;
            t.read_len=1; /* A real one-byte read, not a zero-length transfer. */
            if(ioctl(fd,OS_I2C_TRANSFER,(int)(uintptr_t)&t)==0) {
                printf("0x%02x ",a); found++;
            }
        }
        if(!found) puts("no devices found"); else putchar('\n');
        close(fd);return 0;
    }
    int addr;
    if(argc<4 || number(argv[2],&addr)<0 || addr<8 || addr>0x77) {usage();close(fd);return 1;}
    t.address=(uint8_t)addr;
    int n;
    if(!strcmp(argv[1],"read") || !strcmp(argv[1],"xfer")) {
        if(number(argv[3],&n)<0 || n<1 || n>PICONIX_I2C_MAX_TRANSFER) {usage();close(fd);return 1;}
        t.read_len=(uint8_t)n;
        if(!strcmp(argv[1],"read") && argc!=4) {usage();close(fd);return 1;}
    } else if(strcmp(argv[1],"write")) {usage();close(fd);return 1;}
    int first=!strcmp(argv[1],"write")?3:4;
    if(argc-first>PICONIX_I2C_MAX_TRANSFER || (!strcmp(argv[1],"write") && argc==first)) {usage();close(fd);return 1;}
    for(int i=first;i<argc;i++) {
        if(number(argv[i],&n)<0) {usage();close(fd);return 1;}
        t.write_data[t.write_len++]=(uint8_t)n;
    }
    int rc=ioctl(fd,OS_I2C_TRANSFER,(int)(uintptr_t)&t);
    close(fd);
    if(rc<0) {printf("i2cctl: transfer failed (%d)\n",rc);return 1;}
    for(int i=0;i<t.read_len;i++)printf("%02x%s",t.read_data[i],i+1==t.read_len?"\n":" ");
    return 0;
}
