/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Omikuji: countdown, random fortune, then exit. */
#include <stdint.h>
#include <stddef.h>
#include "unistd.h"
#include "fcntl.h"
static void print(const char *s) {
    unsigned n=0;
    while(s[n]) ++n;
    while(n) {
        int rc=write(1, s, n);
        if(rc<0) return;
        if(rc==0) { sleep(1); continue; }
        s+=(unsigned)rc; n-=(unsigned)rc;
    }
}
static unsigned random_from_device(void) {
    static const char path[]="/dev/rand";
    uint32_t value=0;
    int fd=open(path, 0);
    if(fd<0) return 0;
    int n=read(fd, &value, sizeof(value));
    close(fd);
    return n==(int)sizeof(value) ? value : 0;
}
/* Calculate value modulo 6 without a hardware divide or libgcc. */
static unsigned mod6(uint32_t value) {
    unsigned rem=0;
    for(int bit=31;bit>=0;--bit) {
        rem=(rem<<1)|((value>>(unsigned)bit)&1u);
        if(rem>=6u) rem-=6u;
    }
    return rem;
}
int main(int argc, char **argv) {
    /* Fixed-width strings avoid absolute address entries in a pointer table. */
    static const char fortunes[][9] = {
        "DAIKICHI", "CHUKICHI", "SHOKICHI", "KICHI", "SUEKICHI", "KYO"
    };
    int seconds=0;
    if(argc>1 && argv[1]) {
        const char *p=argv[1];
        while(*p>='0' && *p<='9') { seconds=seconds*10+(*p++-'0'); }
        if(*p || seconds>31) seconds=0;
    }
    if(seconds==31) seconds=10; /* no interactive stdin in background */
    if(seconds==0) {
        print("Countdown seconds (1-30): ");
        char input[16];
        int len=read(0, input, sizeof(input));
        if(len<0) {
            /* A background job must not consume the shell's terminal input. */
            seconds=10;
        } else {
            unsigned value=0;
            int valid=0;
            for(int i=0;i<len;i++) {
                if(input[i]=='\r' || input[i]=='\n') break;
                if(input[i]<'0' || input[i]>'9') { value=0; break; }
                value=value*10u+(unsigned)(input[i]-'0');
                if(value>30) { value=0; break; }
                valid=1;
            }
            if(!valid || value<1 || value>30) {
                print("Invalid seconds (use 1-30).\r\n");
                _exit(1);
                for(;;) {}
            }
            seconds=(int)value;
        }
    }
    print("\t");
    for(int n=seconds;n>=1;n--) {
        char item[6];
        unsigned len=0;
        if(n>=10) { unsigned tens=0, ones=(unsigned)n; while(ones>=10) { ones-=10; tens++; } item[len++]=(char)('0'+tens); item[len++]=(char)('0'+ones); }
        else item[len++]=(char)('0'+n);
        item[len++]='.'; item[len++]='.'; item[len++]='.';
        unsigned sent=0;
        while(sent<len) {
            int rc=write(1, (item+sent), len-sent);
            if(rc<0) break;
            if(rc==0) { sleep(1); continue; }
            sent+=(unsigned)rc;
        }
        sleep(1000);
    }
    unsigned value=random_from_device();
    print(" (*^_^*) ");
    print(fortunes[mod6(value)]);
    print("!\r\n");
    _exit(0);
    for(;;) sleep(1000);
}
