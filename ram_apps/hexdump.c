/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Hex/ASCII dump: hexdump [file [offset [count]]]. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>

static void hex(unsigned value, int digits)
{
    const char chars[] = "0123456789abcdef";
    for (int shift = (digits - 1) * 4; shift >= 0; shift -= 4)
        putchar(chars[(value >> shift) & 15u]);
}

static int number(const char *s, unsigned *value)
{
    char *end;
    if (!*s || *s=='-') return -1;
    long n=strtol(s,&end,0);
    if (*end || n<0) return -1;
    *value=(unsigned long)n;
    return 0;
}

int main(int argc, char *argv[])
{
    /* The shell passes its argument string as one argument. */
    char args[64];
    char *parts[3]={0,0,0};
    int count=0;
    if(argc>1 && argv[1]) {
        if(strlen(argv[1])>=sizeof args) { puts("hexdump: arguments too long"); return 1; }
        strcpy(args,argv[1]);
        char *s=args;
        while(*s) {
            while(*s==' ') s++;
            if(!*s) break;
            if(count==3) { puts("usage: hexdump [file [offset [count]]]"); return 1; }
            parts[count++]=s;
            while(*s && *s!=' ') s++;
            if(*s) *s++=0;
        }
    }
    int fd=0;
    if(count) {
        fd=open(parts[0],O_RDONLY);
        if(fd<0) { printf("hexdump: cannot open %s\n",parts[0]); return 1; }
    }
    unsigned offset=0,remaining=0;
    int limited=count==3;
    if((count>=2 && number(parts[1],&offset)<0) ||
       (limited && number(parts[2],&remaining)<0)) {
        puts("hexdump: invalid offset or count");
        if(fd) close(fd);
        return 1;
    }
    if(count>=2 && lseek(fd,(long)offset,SEEK_SET)<0) {
        puts("hexdump: cannot seek");
        if(fd) close(fd);
        return 1;
    }
    /* /dev/mem needs an explicit size to avoid dumping all memory. */
    if(count && strcmp(parts[0],"/dev/mem")==0 && !limited) {
        puts("hexdump: /dev/mem requires offset and count");
        close(fd);
        return 1;
    }
    unsigned char data[16];
    int n=0;
    while(!limited || remaining) {
        size_t want=sizeof data;
        if(limited && remaining<want) want=remaining;
        n=read(fd,data,want);
        if(n<=0) break;
        hex(offset,8);
        printf("  ");
        for(int i=0;i<16;i++) {
            if(i<n) hex(data[i],2);
            else printf("  ");
            putchar(' ');
            if(i==7) putchar(' ');
        }
        printf(" | ");
        for(int i=0;i<n;i++) {
            unsigned char c=data[i];
            putchar(c>=32 && c<=126?c:'.');
        }
        putchar('\n');
        offset+=(unsigned)n;
        if(limited) remaining-=(unsigned)n;
    }
    if(fd) close(fd);
    if(n<0) { puts("hexdump: read error"); return 1; }
    return 0;
}
