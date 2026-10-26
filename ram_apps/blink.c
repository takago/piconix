/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Standalone slot-linked Blink image. No SDK, libc, globals or flash references. */
#include <stdint.h>

enum { SYS_EXIT=3, SYS_SLEEP=4, SYS_OPEN=6, SYS_WRITE=9, SYS_IOCTL=10 };
enum { OS_O_WRONLY=1, OS_GPIO_OUTPUT=2 };

static inline int svc3(int n, uintptr_t a, uintptr_t b, uintptr_t c) {
    register uintptr_t r0 __asm("r0") = (uintptr_t)n;
    register uintptr_t r1 __asm("r1") = a;
    register uintptr_t r2 __asm("r2") = b;
    register uintptr_t r3 __asm("r3") = c;
    __asm volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "memory");
    return (int)r0;
}

__attribute__((noreturn,used,section(".text.app_entry")))
void app_entry(void) {
    int fd=svc3(SYS_OPEN,(uintptr_t)"/dev/gpio/25",OS_O_WRONLY,0);
    if(fd<0) { svc3(SYS_EXIT,0,0,0); for(;;) {} }
    svc3(SYS_IOCTL,(uintptr_t)fd,OS_GPIO_OUTPUT,0);
    for(;;) {
        char on='1',off='0';
        svc3(SYS_WRITE,(uintptr_t)fd,(uintptr_t)&on,1);
        svc3(SYS_SLEEP,500,0,0);
        svc3(SYS_WRITE,(uintptr_t)fd,(uintptr_t)&off,1);
        svc3(SYS_SLEEP,500,0,0);
    }
}
