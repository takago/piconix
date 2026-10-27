/* Minimal, freestanding user API; no dependency on host libc. */
#include "unistd.h"
#include <stdint.h>
static inline int call(int n, uintptr_t a, uintptr_t b, uintptr_t c) {
    register uintptr_t r0 __asm("r0") = (uintptr_t)n;
    register uintptr_t r1 __asm("r1") = a;
    register uintptr_t r2 __asm("r2") = b;
    register uintptr_t r3 __asm("r3") = c;
    __asm volatile("svc #0" : "+r"(r0) : "r"(r1), "r"(r2), "r"(r3) : "memory");
    return (int)r0;
}
int getpid(void) { return call(1,0,0,0); }
int kill(int pid) { return call(2,(uintptr_t)pid,0,0); }
void _exit(int status) { call(3,(uintptr_t)status,0,0); for(;;) {} }
int sleep(unsigned ms) { return call(4,ms,0,0); }
int yield(void) { return call(5,0,0,0); }
int open(const char *path,int flags) { return call(6,(uintptr_t)path,(uintptr_t)flags,0); }
int close(int fd) { return call(7,(uintptr_t)fd,0,0); }
long lseek(int fd,long offset,int whence) { return call(21,(uintptr_t)fd,(uintptr_t)offset,(uintptr_t)whence); }
int read(int fd,void *p,size_t n) { return call(8,(uintptr_t)fd,(uintptr_t)p,n); }
int write(int fd,const void *p,size_t n) { return call(9,(uintptr_t)fd,(uintptr_t)p,n); }
int ioctl(int fd,int cmd,int value) { return call(10,(uintptr_t)fd,(uintptr_t)cmd,(uintptr_t)value); }
int waitpid(int pid,int *status,int options) { return call(13,(uintptr_t)pid,(uintptr_t)status,(uintptr_t)options); }
int unlink(const char *path) { return call(17,(uintptr_t)path,0,0); }
int dup2(int oldfd,int newfd) { return call(18,(uintptr_t)oldfd,(uintptr_t)newfd,0); }

int spawn(int program,unsigned arg,const char *path) { return call(12,(uintptr_t)program,arg,(uintptr_t)path); }
int tty_spawn(int tty,int program) { return call(19,(uintptr_t)tty,(uintptr_t)program,0); }
int set_foreground(int pid) { return call(14,(uintptr_t)pid,0,0); }
int console_ready(void) { return call(16,0,0,0); }
int get_tty(void) { return call(20,0,0,0); }
