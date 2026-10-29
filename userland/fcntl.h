/* PicoNIX open flags. These values are not host POSIX flag values. */
#ifndef PICONIX_FCNTL_H
#define PICONIX_FCNTL_H
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_NONBLOCK 4
#define O_CREAT 8
#define O_TRUNC 16
#define O_APPEND 32
#define F_GETFL 1
#define F_SETFL 2
int fcntl(int fd, int cmd, int value);
#endif
