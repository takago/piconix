/* PicoNIX user-space syscall interfaces (small POSIX-like subset). */
#ifndef PICONIX_UNISTD_H
#define PICONIX_UNISTD_H
#include <stddef.h>
#include "../include/piconix.h"
#include "../include/piconix_dirent.h"
#define FB_ENABLE  0x100
/* PicoNIX console ioctl commands (minimal, not POSIX termios). */
#define OS_TTY_GET_CANON 0x200
#define OS_TTY_SET_CANON 0x201
#define OS_TTY_GET_ECHO 0x202
#define OS_TTY_SET_ECHO 0x203
#define OS_TTY_GET_ONLCR 0x204
#define OS_TTY_SET_ONLCR 0x205
#define OS_TTY_PERSIST 0x206
#define OS_TTY_CLAIM   0x207
#define OS_TTY_RELEASE 0x208
#define FB_DISABLE 0x101
#define FB_STATUS  0x102
int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);
int close(int fd);
/* Small BSD-style IPv4 socket subset. Network byte order for ports/addresses.
 * Returns negative OS_* error codes directly (PicoNIX has no errno).
 * accept() and recvfrom() return OS_EAGAIN when no data/connection is ready.
 * connect() starts asynchronously; use tcp_status() to poll completion.
 * bind(TCP) starts listening immediately; listen() is a compatibility no-op.
 */
#define AF_INET 2
#define SOCK_STREAM 1
#define SOCK_DGRAM 2
#define INADDR_ANY 0u
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
typedef unsigned int socklen_t;
struct in_addr { uint32_t s_addr; };
struct sockaddr { uint16_t sa_family; char sa_data[14]; };
struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    struct in_addr sin_addr;
    unsigned char sin_zero[8];
};
static inline uint16_t htons(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
#define ntohs htons
int socket(int domain, int type, int protocol);
int bind(int fd, const struct sockaddr *addr, socklen_t len);
int listen(int fd, int backlog);
int accept(int fd, struct sockaddr *addr, socklen_t *len);
int connect(int fd, const struct sockaddr *addr, socklen_t len);
/* Connected TCP sockets: flags must be zero. */
int send(int fd, const void *buf, size_t len, int flags);
int recv(int fd, void *buf, size_t len, int flags);
int sendto(int fd, const void *buf, size_t len, int flags,
           const struct sockaddr *to, socklen_t tolen);
int recvfrom(int fd, void *buf, size_t len, int flags,
             struct sockaddr *from, socklen_t *fromlen);
/* PicoNIX-specific nonblocking TCP connect status: 0=pending, 1=ready. */
int tcp_status(int fd);
/* Legacy low-level interfaces retained for existing applications. */
int bind_udp(int fd, unsigned port);
int sendto_udp(int fd, const piconix_udp_packet_t *packet);
int recvfrom_udp(int fd, piconix_udp_packet_t *packet);
int tcp_listen(int fd, unsigned port);
int tcp_accept(int fd);
int tcp_connect(int fd, const piconix_tcp_endpoint_t *endpoint);
/* Seek to an absolute offset; PicoNIX currently supports SEEK_SET only. */
#define SEEK_SET 0
long lseek(int fd, long offset, int whence);
int open(const char *path, int flags);
int unlink(const char *path);
/* 1: entry returned, 0: end of directory, negative: error. */
int getdents(int fd, piconix_dirent_t *entry, size_t size);
int dup2(int oldfd, int newfd);
int getpid(void);
unsigned flash_id(void);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int kill(int pid);
int msleep(unsigned milliseconds); /* PicoNIX: delay in milliseconds. */
int yield(void);
#define WNOHANG 1
int waitpid(int pid, int *status, int options);
int ioctl(int fd, int command, int value);
/* PicoNIX-specific process and terminal operations. */
int spawn(int program, int argc, const char *const argv[]);
int tty_spawn(int tty, int program);
int spawn_tmp(const char *path);
int set_foreground(int pid);
int console_ready(void);
int get_tty(void);
void _exit(int status) __attribute__((noreturn));
#endif
