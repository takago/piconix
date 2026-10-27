/* PicoNIX user-space syscall interfaces (small POSIX-like subset). */
#ifndef PICONIX_UNISTD_H
#define PICONIX_UNISTD_H
#include <stddef.h>
int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);
int close(int fd);
/* Seek to an absolute offset; PicoNIX currently supports SEEK_SET only. */
#define SEEK_SET 0
long lseek(int fd, long offset, int whence);
int open(const char *path, int flags);
int unlink(const char *path);
int dup2(int oldfd, int newfd);
int getpid(void);
int kill(int pid);
int sleep(unsigned milliseconds); /* PicoNIX: milliseconds, not POSIX seconds. */
int yield(void);
int waitpid(int pid, int *status, int options);
int ioctl(int fd, int command, int value);
/* PicoNIX-specific process and terminal operations. */
int spawn(int program, unsigned arg, const char *path);
int tty_spawn(int tty, int program);
int set_foreground(int pid);
int console_ready(void);
int get_tty(void);
void _exit(int status) __attribute__((noreturn));
#endif
