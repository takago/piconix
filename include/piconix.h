/* カーネル側のタスク制御・デバイス操作APIと共通の制約を定義する． */
#ifndef PICONIX_H
#define PICONIX_H
#include <stdint.h>
#include <stddef.h>
#ifndef OS_MAX_TASKS
#define OS_MAX_TASKS 6
#endif
#define OS_TASK_BYTES (24u * 1024u)
#define OS_O_RDONLY 0
#define OS_O_WRONLY 1
#define OS_O_RDWR 2
#define OS_O_NONBLOCK 4
/* PicoNIX独自の簡易コンソールioctl．POSIXのtermiosとは異なる． */
#define OS_TTY_GET_CANON 0x200
#define OS_TTY_SET_CANON 0x201
#define OS_TTY_GET_ECHO 0x202
#define OS_TTY_SET_ECHO 0x203
#define OS_TTY_GET_ONLCR 0x204
#define OS_TTY_SET_ONLCR 0x205
#define OS_TTY_PERSIST 0x206
#define OS_TTY_CLAIM   0x207
#define OS_TTY_RELEASE 0x208
#define OS_O_CREAT 8
#define OS_O_TRUNC 16
#define OS_O_APPEND 32
#define OS_ENOSPC (-28)
#define OS_EBUSY (-16)
#define OS_GPIO_INPUT 1
#define OS_GPIO_OUTPUT 2
#define OS_GPIO_PULL_UP 3
#define OS_GPIO_PULL_DOWN 4
#define OS_GPIO_NO_PULL 5
#define OS_FB_ENABLE  0x100
#define OS_FB_DISABLE 0x101
#define OS_FB_STATUS  0x102
#define OS_WNOHANG 1
#define OS_EINVAL (-22)
#define OS_EBADF (-9)
#define OS_EAGAIN (-11)
#define OS_ENOSYS (-38)
#define OS_ESRCH (-3)
#define OS_EMFILE (-24)
/* IPv4アドレスはlwIPと同じネットワークバイト順のuint32_t表現を用いる． */
typedef struct {
    const void *data;
    uint32_t length;
    uint32_t peer_ip;
    uint16_t peer_port;
    uint16_t reserved;
} piconix_udp_packet_t;
typedef void (*os_entry_t)(void);
#define OS_TASK_UNUSED 0
#define OS_TASK_READY 1
#define OS_TASK_SLEEPING 2
#define OS_TASK_DEAD 3
#define OS_TASK_BLOCKED 4
#define OS_EINTR (-4)
#define OS_EIO (-5)
#define OS_ENOENT (-2)
/* IPv4ソケットAPI．TCPのサーバ・クライアントとUDPを限定的に扱う． */
#define OS_AF_INET 2
#define OS_SOCK_STREAM 1
#define OS_SOCK_DGRAM 2
int os_socket(int domain,int type,int protocol);
int os_tcp_listen(int fd,uint16_t port);
/* ノンブロッキングで，接続FDまたはOS_EAGAINを返す． */
int os_tcp_accept(int fd);
/* TCP接続先のIPv4表現はUDPのpeer_ipと共通である． */
typedef struct { uint32_t peer_ip; uint16_t peer_port; } piconix_tcp_endpoint_t;
int os_tcp_connect(int fd,const piconix_tcp_endpoint_t *endpoint);
int os_tcp_status(int fd);
void os_init(void);
// SDKのUSB標準入出力をアイドル側でポーリングする．
void os_console_poll(void);
void os_init_idle(os_entry_t entry);
/* 診断用にアイドル／CYW43スタックの使用高水位を概算する． */
unsigned os_idle_stack_used_bytes(void);
unsigned os_idle_stack_capacity_bytes(void);
int os_spawn(os_entry_t entry);
int os_spawn_named(os_entry_t entry, const char *name);
int os_spawn_program(int program_id);
int os_start_program(int program_id); // スケジューラ開始前に起動するための関数．
void os_register_program(int program_id, os_entry_t entry, const char *name);
// Flashのスロット向けイメージを登録し，実行時に割当て済みRAMへコピーする．
// この登録だけでは入口アドレスの変更やRAM実行の有効化はしない．
int os_register_ram_image(int program_id, const void *image, size_t size,
                          size_t zero_start, size_t zero_size);
void os_start(void) __attribute__((noreturn));
int os_getpid(void);
int os_get_tty(void);
int os_kill(int pid);
int os_waitpid(int pid, int *status, int options);
int os_set_foreground(int pid);
void os_exit(void) __attribute__((noreturn));
void os_msleep(uint32_t milliseconds);
void os_yield(void);
int os_open(const char *path, int flags);
int os_close(int fd);
int os_unlink(const char *path);
int os_pipe(int fds[2]);
int os_dup2(int oldfd,int newfd);
int os_read(int fd, void *buf, size_t n);
int os_write(int fd, const void *buf, size_t n);
int os_ioctl(int fd, int cmd, int value);
int os_fcntl(int fd, int cmd, int value);
#endif
