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
int chdir(const char *path) { return call(22,(uintptr_t)path,0,0); }
char *getcwd(char *buf,size_t size) { return call(23,(uintptr_t)buf,size,0)<0 ? 0 : buf; }
int getpid(void) { return call(1,0,0,0); }
unsigned flash_id(void) { return (unsigned)call(27,0,0,0); }
int kill(int pid) { return call(2,(uintptr_t)pid,0,0); }
void _exit(int status) { call(3,(uintptr_t)status,0,0); for(;;) {} }
int msleep(unsigned ms) { return call(4,ms,0,0); }
int yield(void) { return call(5,0,0,0); }
int open(const char *path,int flags) { return call(6,(uintptr_t)path,(uintptr_t)flags,0); }
int close(int fd) { return call(7,(uintptr_t)fd,0,0); }
long lseek(int fd,long offset,int whence) { return call(21,(uintptr_t)fd,(uintptr_t)offset,(uintptr_t)whence); }
int read(int fd,void *p,size_t n) { return call(8,(uintptr_t)fd,(uintptr_t)p,n); }
int write(int fd,const void *p,size_t n) { return call(9,(uintptr_t)fd,(uintptr_t)p,n); }
int ioctl(int fd,int cmd,int value) { return call(10,(uintptr_t)fd,(uintptr_t)cmd,(uintptr_t)value); }
int fcntl(int fd,int cmd,int value) { return call(24,(uintptr_t)fd,(uintptr_t)cmd,(uintptr_t)value); }
int waitpid(int pid,int *status,int options) { return call(13,(uintptr_t)pid,(uintptr_t)status,(uintptr_t)options); }
int getdents(int fd,piconix_dirent_t *entry,size_t size) { return call(26,(uintptr_t)fd,(uintptr_t)entry,size); }
int unlink(const char *path) { return call(17,(uintptr_t)path,0,0); }
int dup2(int oldfd,int newfd) { return call(18,(uintptr_t)oldfd,(uintptr_t)newfd,0); }

int spawn(int program,int argc,const char *const argv[]) { return call(12,(uintptr_t)program,(uintptr_t)argc,(uintptr_t)argv); }
int tty_spawn(int tty,int program) { return call(19,(uintptr_t)tty,(uintptr_t)program,0); }
int set_foreground(int pid) { return call(14,(uintptr_t)pid,0,0); }
int console_ready(void) { return call(16,0,0,0); }
int get_tty(void) { return call(20,0,0,0); }

int spawn_tmp(const char *path) { return call(25,(uintptr_t)path,0,0); }

int socket(int domain,int type,int protocol) { return call(28,domain,type,protocol); }
int bind_udp(int fd,unsigned port) { return call(29,fd,port,0); }
int sendto_udp(int fd,const piconix_udp_packet_t *packet) { return call(30,fd,(uintptr_t)packet,0); }
int recvfrom_udp(int fd,piconix_udp_packet_t *packet) { return call(31,fd,(uintptr_t)packet,0); }

/* Keep syscall numbers synchronized with kernel/kernel.c. */
int tcp_listen(int fd,unsigned port) { return call(32,fd,port,0); }
int tcp_connect(int fd,const piconix_tcp_endpoint_t *ep) { return call(33,fd,(uintptr_t)ep,0); }
int tcp_status(int fd) { return call(34,fd,0,0); }
int tcp_accept(int fd) { return call(35,fd,0,0); }

/* Minimal BSD-style adapters; no dynamic allocation and no libc dependency. */
static int ipv4_addr(const struct sockaddr *addr, socklen_t len,
                     const struct sockaddr_in **out) {
    if (!addr || len < sizeof(struct sockaddr_in)) return OS_EINVAL;
    *out = (const struct sockaddr_in *)addr;
    return (*out)->sin_family == AF_INET ? 0 : OS_EINVAL;
}
int bind(int fd, const struct sockaddr *addr, socklen_t len) {
    const struct sockaddr_in *in;
    int r = ipv4_addr(addr, len, &in);
    if (r < 0) return r;
    unsigned port = ntohs(in->sin_port);
    if (!port || in->sin_addr.s_addr != INADDR_ANY) return OS_EINVAL;
    /* The underlying TCP syscall combines bind and listen. */
    r = bind_udp(fd, port);
    if (r == OS_EBADF || r == OS_EINVAL) return tcp_listen(fd, port);
    return r;
}
int listen(int fd, int backlog) {
    (void)fd;
    return backlog < 0 ? OS_EINVAL : 0; /* TCP bind() already listens. */
}
int accept(int fd, struct sockaddr *addr, socklen_t *len) {
    if (addr || len) return OS_EINVAL; /* Peer address query not yet supported. */
    return tcp_accept(fd);
}
int connect(int fd, const struct sockaddr *addr, socklen_t len) {
    const struct sockaddr_in *in;
    int r = ipv4_addr(addr, len, &in);
    if (r < 0) return r;
    piconix_tcp_endpoint_t ep = {in->sin_addr.s_addr, ntohs(in->sin_port)};
    return tcp_connect(fd, &ep);
}
int send(int fd, const void *buf, size_t len, int flags) {
    if (flags || (len && !buf)) return OS_EINVAL;
    return write(fd, buf, len);
}
int recv(int fd, void *buf, size_t len, int flags) {
    if (flags || (len && !buf)) return OS_EINVAL;
    return read(fd, buf, len);
}
int sendto(int fd, const void *buf, size_t len, int flags,
           const struct sockaddr *to, socklen_t tolen) {
    const struct sockaddr_in *in;
    int r = ipv4_addr(to, tolen, &in);
    if (r < 0) return r;
    if (flags || len > UINT32_MAX || (len && !buf)) return OS_EINVAL;
    piconix_udp_packet_t pkt = {buf, (uint32_t)len, in->sin_addr.s_addr,
                                ntohs(in->sin_port), 0};
    return sendto_udp(fd, &pkt);
}
int recvfrom(int fd, void *buf, size_t len, int flags,
             struct sockaddr *from, socklen_t *fromlen) {
    if (flags || len > UINT32_MAX || (len && !buf)) return OS_EINVAL;
    if (from && (!fromlen || *fromlen < sizeof(struct sockaddr_in))) return OS_EINVAL;
    piconix_udp_packet_t pkt = {buf, (uint32_t)len, 0, 0, 0};
    int r = recvfrom_udp(fd, &pkt);
    if (r >= 0 && from) {
        struct sockaddr_in *in = (struct sockaddr_in *)from;
        *in = (struct sockaddr_in){0};
        in->sin_family = AF_INET;
        in->sin_port = htons(pkt.peer_port);
        in->sin_addr.s_addr = pkt.peer_ip;
        *fromlen = sizeof(*in);
    }
    return r;
}
