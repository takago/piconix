# System Call Reference (Current Implementation)

This table is checked against the `SYS_*` enumeration in `kernel/kernel.c` and wrappers in `userland/syscalls.c`. Numbers describe the internal ABI, not full POSIX compatibility.

If you are writing a C application, start with the [programming guide](PROGRAMMING.md). This page is a **low-level ABI reference** for assembly programming and kernel inspection.

## Calling convention

- `svc #0`: `r0` = call number, `r1`–`r3` = arguments, `r0` = return value.
- Numbers 11 and 15 are reserved, not callable services.
- The kernel dispatcher is `os_svc_dispatch()`; user wrappers are in `userland/syscalls.c`.

## Call numbers

| No. | Kernel constant | User API | Function |
|---:|---|---|---|
| 1 | `SYS_GETPID` | `getpid` | Get the calling task PID |
| 2 | `SYS_KILL` | `kill` | Terminate another task by PID |
| 3 | `SYS_EXIT` | `_exit` | Terminate the current task |
| 4 | `SYS_SLEEP` | `msleep` | Sleep for a specified time in milliseconds |
| 5 | `SYS_YIELD` | `yield` | Yield the CPU to the scheduler |
| 6 | `SYS_OPEN` | `open` | Open a file or device and obtain an FD |
| 7 | `SYS_CLOSE` | `close` | Close an open FD |
| 8 | `SYS_READ` | `read` | Read from an FD |
| 9 | `SYS_WRITE` | `write` | Write data to an FD |
| 10 | `SYS_IOCTL` | `ioctl` | Perform device or TTY control |
| 11 | `SYS_RESERVED_TASK_INFO` | `reserved (task info)` | Reserved; no callable service |
| 12 | `SYS_SPAWN` | `spawn` | Start a registered user program |
| 13 | `SYS_WAITPID` | `waitpid` | Wait for a child task to finish |
| 14 | `SYS_SET_FG` | `set_foreground` | Set foreground task ownership for the TTY |
| 15 | `SYS_RESERVED_GETARG` | `reserved (getarg)` | Reserved; no callable service |
| 16 | `SYS_CONSOLE_READY` | `console_ready` | Check whether the console connection is ready |
| 17 | `SYS_UNLINK` | `unlink` | Remove a writable file |
| 18 | `SYS_DUP2` | `dup2` | Duplicate an FD into a specified FD number |
| 19 | `SYS_TTY_SPAWN` | `tty_spawn` | Start a shell or getty on a specified TTY |
| 20 | `SYS_GET_TTY` | `get_tty` | Obtain the current task’s TTY identifier |
| 21 | `SYS_LSEEK` | `lseek` | Move the read/write position of a file |
| 22 | `SYS_CHDIR` | `chdir` | Change the current working directory |
| 23 | `SYS_GETCWD` | `getcwd` | Obtain the current working directory |
| 24 | `SYS_FCNTL` | `fcntl` | Get or set FD flags such as nonblocking I/O |
| 25 | `SYS_SPAWN_TMP` | `spawn_tmp` | Start a flat Thumb program stored in /tmp |
| 26 | `SYS_GETDENTS` | `getdents` | Read directory entries |
| 27 | `SYS_FLASH_ID` | `flash_id` | Read the Flash identifier |
| 28 | `SYS_SOCKET` | `socket` | Create a TCP or UDP socket |
| 29 | `SYS_BIND` | `bind_udp / TCP listen adapter` | Bind a UDP port or initiate TCP listening via adapter |
| 30 | `SYS_SENDTO` | `sendto_udp` | Send a UDP datagram |
| 31 | `SYS_RECVFROM` | `recvfrom_udp` | Receive a UDP datagram |
| 32 | `SYS_LISTEN_TCP` | `tcp_listen` | Start TCP listening |
| 33 | `SYS_CONNECT_TCP` | `tcp_connect` | Connect to a TCP peer |
| 34 | `SYS_TCP_STATUS` | `tcp_status` | Query TCP connection state |
| 35 | `SYS_ACCEPT_TCP` | `tcp_accept` | Accept an incoming TCP connection |
| 36 | `SYS_PIPE` | `pipe` | Create a pipe and its two FDs |
| 37 | `SYS_DUP` | `dup` | Duplicate an FD to a new available FD |
| 38 | `SYS_PROGRAM_ID` | `program_id` | Resolve a registered program name to its ID |

## Important limitations

- `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, and `recvfrom()` are BSD-style adapters in `userland/syscalls.c`; they are not each distinct syscall numbers.
- For TCP, `bind()` initiates listening internally; `listen()` does not implement POSIX backlog handling, and `accept()` does not return peer addresses.
- `pipe()`, `dup()`, and `dup2()` provide descriptor operations without full POSIX semantics.
- Assembly programs set the call number in `r0` and execute `svc #0`; see [ASM.md](ASM.md).
