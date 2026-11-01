# System Call Reference (Current Implementation)

This table is checked against the `SYS_*` enumeration in `kernel/kernel.c` and wrappers in `userland/syscalls.c`. Numbers describe the internal ABI, not full POSIX compatibility.

If you are writing a C application, start with the [programming guide](PROGRAMMING.md). This page is a **low-level ABI reference** for assembly programming and kernel inspection.

## Calling convention

- `svc #0`：`r0` = call number，`r1`–`r3` = arguments，`r0` = return value．
- Numbers 11 and 15 are reserved, not callable services.
- The kernel dispatcher is `os_svc_dispatch()`; user wrappers are in `userland/syscalls.c`.

## Call numbers

| No. | Kernel constant | User API / meaning |
|---:|---|---|
| 1 | `SYS_GETPID` | `getpid` |
| 2 | `SYS_KILL` | `kill` |
| 3 | `SYS_EXIT` | `_exit` |
| 4 | `SYS_SLEEP` | `msleep` |
| 5 | `SYS_YIELD` | `yield` |
| 6 | `SYS_OPEN` | `open` |
| 7 | `SYS_CLOSE` | `close` |
| 8 | `SYS_READ` | `read` |
| 9 | `SYS_WRITE` | `write` |
| 10 | `SYS_IOCTL` | `ioctl` |
| 11 | `SYS_RESERVED_TASK_INFO` | `reserved (task info)` |
| 12 | `SYS_SPAWN` | `spawn` |
| 13 | `SYS_WAITPID` | `waitpid` |
| 14 | `SYS_SET_FG` | `set_foreground` |
| 15 | `SYS_RESERVED_GETARG` | `reserved (getarg)` |
| 16 | `SYS_CONSOLE_READY` | `console_ready` |
| 17 | `SYS_UNLINK` | `unlink` |
| 18 | `SYS_DUP2` | `dup2` |
| 19 | `SYS_TTY_SPAWN` | `tty_spawn` |
| 20 | `SYS_GET_TTY` | `get_tty` |
| 21 | `SYS_LSEEK` | `lseek` |
| 22 | `SYS_CHDIR` | `chdir` |
| 23 | `SYS_GETCWD` | `getcwd` |
| 24 | `SYS_FCNTL` | `fcntl` |
| 25 | `SYS_SPAWN_TMP` | `spawn_tmp` |
| 26 | `SYS_GETDENTS` | `getdents` |
| 27 | `SYS_FLASH_ID` | `flash_id` |
| 28 | `SYS_SOCKET` | `socket` |
| 29 | `SYS_BIND` | `bind_udp / TCP listen adapter` |
| 30 | `SYS_SENDTO` | `sendto_udp` |
| 31 | `SYS_RECVFROM` | `recvfrom_udp` |
| 32 | `SYS_LISTEN_TCP` | `tcp_listen` |
| 33 | `SYS_CONNECT_TCP` | `tcp_connect` |
| 34 | `SYS_TCP_STATUS` | `tcp_status` |
| 35 | `SYS_ACCEPT_TCP` | `tcp_accept` |
| 36 | `SYS_PIPE` | `pipe` |
| 37 | `SYS_DUP` | `dup` |
| 38 | `SYS_PROGRAM_ID` | `program_id` |

## Important limitations

- `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, and `recvfrom()` are BSD-style adapters in `userland/syscalls.c`; they are not each distinct syscall numbers.
- For TCP, `bind()` initiates listening internally; `listen()` does not implement POSIX backlog handling, and `accept()` does not return peer addresses.
- `pipe()`, `dup()`, and `dup2()` provide descriptor operations without full POSIX semantics.
- Assembly programs set the call number in `r0` and execute `svc #0`; see [ASM.md](ASM.md).
