/* カーネルとユーザ空間で共有するシステムコール番号表．
 * 番号はSVC ABIで使うため，変更する場合は両側を同時に確認する． */
/* カーネル・ユーザ間で共有するシステムコール番号と表示名． */
#ifndef PICONIX_SYSCALL_H
#define PICONIX_SYSCALL_H

#define PICONIX_SYSCALL_TABLE(X) \
    X(SYS_GETPID,        1,  "getpid") \
    X(SYS_KILL,          2,  "kill") \
    X(SYS_EXIT,          3,  "exit") \
    X(SYS_SLEEP,         4,  "sleep") \
    X(SYS_YIELD,         5,  "yield") \
    X(SYS_OPEN,          6,  "open") \
    X(SYS_CLOSE,         7,  "close") \
    X(SYS_READ,          8,  "read") \
    X(SYS_WRITE,         9,  "write") \
    X(SYS_IOCTL,         10, "ioctl") \
    X(SYS_RESERVED_TASK_INFO, 11, "reserved") \
    X(SYS_SPAWN,         12, "spawn") \
    X(SYS_WAITPID,       13, "waitpid") \
    X(SYS_SET_FG,        14, "set_fg") \
    X(SYS_RESERVED_GETARG, 15, "reserved") \
    X(SYS_CONSOLE_READY, 16, "console_ready") \
    X(SYS_UNLINK,        17, "unlink") \
    X(SYS_DUP2,          18, "dup2") \
    X(SYS_TTY_SPAWN,     19, "tty_spawn") \
    X(SYS_GET_TTY,       20, "get_tty") \
    X(SYS_LSEEK,         21, "lseek") \
    X(SYS_CHDIR,         22, "chdir") \
    X(SYS_GETCWD,        23, "getcwd") \
    X(SYS_FCNTL,         24, "fcntl") \
    X(SYS_SPAWN_TMP,     25, "spawn_tmp") \
    X(SYS_GETDENTS,      26, "getdents") \
    X(SYS_FLASH_ID,      27, "flash_id") \
    X(SYS_SOCKET,        28, "socket") \
    X(SYS_BIND,          29, "bind") \
    X(SYS_SENDTO,        30, "sendto") \
    X(SYS_RECVFROM,      31, "recvfrom") \
    X(SYS_LISTEN_TCP,    32, "listen") \
    X(SYS_CONNECT_TCP,   33, "connect") \
    X(SYS_TCP_STATUS,    34, "tcp_status") \
    X(SYS_ACCEPT_TCP,    35, "accept") \
    X(SYS_PIPE,          36, "pipe") \
    X(SYS_DUP,           37, "dup") \
    X(SYS_PROGRAM_ID,    38, "program_id") \
    X(SYS_SHUTDOWN_TCP,  39, "shutdown") \
    X(SYS_SELECT,        40, "select")

enum {
#define PICONIX_SYSCALL_ENUM(name, number, label) name = number,
    PICONIX_SYSCALL_TABLE(PICONIX_SYSCALL_ENUM)
#undef PICONIX_SYSCALL_ENUM
};

#endif
