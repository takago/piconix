/* ユーザプログラムから利用するファイル・プロセス・通信APIの宣言． */
/* PicoNIXのユーザ空間向けシステムコール宣言．POSIXの一部と似た小さな機能集合である． */
#ifndef PICONIX_UNISTD_H
#define PICONIX_UNISTD_H
#include <stddef.h>
#include "../../include/piconix.h"
#include "../../include/piconix_dirent.h"
#include "../../include/piconix_select.h"
/* ユーザランタイムの全シンボルは各PICイメージ内にリンクする． */
#ifndef PICONIX_HOST_TEST
#pragma GCC visibility push(hidden)
#endif
#define FB_ENABLE  0x100
/* コンソールioctlはPicoNIX独自の簡易APIで，POSIX termiosとは異なる． */
#define OS_TTY_GET_CANON 0x200
#define OS_TTY_SET_CANON 0x201
#define OS_TTY_GET_ECHO 0x202
#define OS_TTY_SET_ECHO 0x203
#define OS_TTY_GET_ONLCR 0x204
#define OS_TTY_SET_ONLCR 0x205
#define OS_TTY_PERSIST 0x206
#define OS_TTY_CLAIM   0x207
#define OS_TTY_RELEASE 0x208
/* Ctrl+Cを前景タスクの中断として扱うか（0で文字として渡す）． */
#define OS_TTY_GET_INTR 0x209
#define OS_TTY_SET_INTR 0x20a
#define FB_DISABLE 0x101
#define FB_STATUS  0x102
/* 小型fd_set: PicoNIXではFDは0〜15．例外FD集合は未対応． */
typedef uint16_t fd_set;
#define FD_ZERO(p) (*(p)=0)
#define FD_SET(fd,p) (*(p)=(uint16_t)(*(p)|(uint16_t)(1u<<(fd))))
#define FD_CLR(fd,p) (*(p)=(uint16_t)(*(p)&(uint16_t)~(1u<<(fd))))
#define FD_ISSET(fd,p) ((*(p)&(uint16_t)(1u<<(fd)))!=0)
struct timeval { long tv_sec; long tv_usec; };
/* timeout=NULL: 無期限．FDはソケット・TTY・パイプだけを監視する． */
int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, const struct timeval *timeout);
int read(int fd, void *buf, size_t count);
int write(int fd, const void *buf, size_t count);
int close(int fd);
/* BSD風IPv4ソケット機能の一部を提供する．ポート・アドレスはネットワークバイト順で，errnoはなく負のOS_*を直接返す．accept・recv・recvfromは既定で待機し，O_NONBLOCK時だけ未準備でOS_EAGAIN．connectは非同期でtcp_statusにより確認する． */
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
/* 16ビット整数のバイト順を入れ替える．ネットワークのビッグエンディアン表現に使う． */
static inline uint16_t htons(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
#define ntohs htons
int socket(int domain, int type, int protocol);
int bind(int fd, const struct sockaddr *addr, socklen_t len);
int listen(int fd, int backlog);
int accept(int fd, struct sockaddr *addr, socklen_t *len);
int connect(int fd, const struct sockaddr *addr, socklen_t len);
/* 接続済みTCPソケットのflagsは0のみ許可する． */
int send(int fd, const void *buf, size_t len, int flags);
int recv(int fd, void *buf, size_t len, int flags);
int sendto(int fd, const void *buf, size_t len, int flags,
           const struct sockaddr *to, socklen_t tolen);
int recvfrom(int fd, void *buf, size_t len, int flags,
             struct sockaddr *from, socklen_t *fromlen);
/* PicoNIX固有の非同期TCP接続状態．0は未完了，1は準備完了を示す． */
int tcp_status(int fd);
/* TCP送信方向だけの半閉鎖．shutdown(fd, SHUT_WR)に相当する． */
#define SHUT_WR 1
int shutdown(int fd, int how);
/* 既存アプリのために下位レベルのAPIも残す． */
int bind_udp(int fd, unsigned port);
int sendto_udp(int fd, const piconix_udp_packet_t *packet);
int recvfrom_udp(int fd, piconix_udp_packet_t *packet);
int tcp_listen(int fd, unsigned port);
int tcp_accept(int fd);
int tcp_connect(int fd, const piconix_tcp_endpoint_t *endpoint);
/* seekの基準は先頭・現在位置・終端（/tmp・/dev/shm・/proc/sym/kernel）を使う． */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
long lseek(int fd, long offset, int whence);
int open(const char *path, int flags);
int unlink(const char *path);
/* 戻り値1は一件取得，0はディレクトリ終端，負値はエラーである． */
int getdents(int fd, piconix_dirent_t *entry, size_t size);
int pipe(int fds[2]);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int getpid(void);
unsigned flash_id(void);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int kill(int pid);
int msleep(unsigned milliseconds); /* ミリ秒単位の遅延． */
int yield(void);
#define WNOHANG 1
int waitpid(int pid, int *status, int options);
int ioctl(int fd, int command, int value);
/* PicoNIX固有のプロセス・TTY操作． */
int program_id(const char *name); /* 登録済み/binのプログラムID，または負のエラーを返す． */
int spawn(int program, int argc, const char *const argv[]);
int spawn_foreground(int program, int argc, const char *const argv[]);
int tty_spawn(int tty, int program);
int spawn_tmp(const char *path);
int spawn_tmp_foreground(const char *path);
int set_foreground(int pid);
int console_ready(void);
int get_tty(void);
void _exit(int status) __attribute__((noreturn));
#ifndef PICONIX_HOST_TEST
#pragma GCC visibility pop
#endif
#endif
