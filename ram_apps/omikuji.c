/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Omikuji: countdown, random fortune, then exit. */
#include <stdint.h>
#include <stddef.h>
enum { SYS_EXIT=3, SYS_SLEEP=4, SYS_OPEN=6, SYS_CLOSE=7,
       SYS_READ=8, SYS_WRITE=9, SYS_GETARG=15 };
static inline int svc3(int n, uintptr_t a, uintptr_t b, uintptr_t c) {
    register uintptr_t r0 __asm("r0")=(uintptr_t)n;
    register uintptr_t r1 __asm("r1")=a;
    register uintptr_t r2 __asm("r2")=b;
    register uintptr_t r3 __asm("r3")=c;
    __asm volatile("svc #0" : "+r"(r0) : "r"(r1),"r"(r2),"r"(r3) : "memory");
    return (int)r0;
}
static void print(const char *s) {
    unsigned n=0;
    while(s[n]) ++n;
    while(n) {
        int rc=svc3(SYS_WRITE,1,(uintptr_t)s,n);
        if(rc<0) return;
        if(rc==0) { svc3(SYS_SLEEP,1,0,0); continue; }
        s+=(unsigned)rc; n-=(unsigned)rc;
    }
}
static unsigned random_from_device(void) {
    static const char path[]="/dev/rand";
    uint32_t value=0;
    int fd=svc3(SYS_OPEN,(uintptr_t)path,0,0);
    if(fd<0) return 0;
    int n=svc3(SYS_READ,(uintptr_t)fd,(uintptr_t)&value,sizeof(value));
    svc3(SYS_CLOSE,(uintptr_t)fd,0,0);
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
__attribute__((used,noreturn,section(".text.app_entry")))
void app_entry(void) {
    /* Fixed-width strings avoid absolute address entries in a pointer table. */
    static const char fortunes[][9] = {
        "DAIKICHI", "CHUKICHI", "SHOKICHI", "KICHI", "SUEKICHI", "KYO"
    };
    int seconds=svc3(SYS_GETARG,0,0,0);
    if(seconds==31) seconds=10; /* no interactive stdin in background */
    if(seconds==0) {
        print("Countdown seconds (1-30): ");
        char input[16];
        int len=svc3(SYS_READ,0,(uintptr_t)input,sizeof(input));
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
                svc3(SYS_EXIT,1,0,0);
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
            int rc=svc3(SYS_WRITE,1,(uintptr_t)(item+sent),len-sent);
            if(rc<0) break;
            if(rc==0) { svc3(SYS_SLEEP,1,0,0); continue; }
            sent+=(unsigned)rc;
        }
        svc3(SYS_SLEEP,1000,0,0);
    }
    unsigned value=random_from_device();
    print(" (*^_^*) ");
    print(fortunes[mod6(value)]);
    print("!\r\n");
    svc3(SYS_EXIT,0,0,0);
    for(;;) svc3(SYS_SLEEP,1000,0,0);
}
