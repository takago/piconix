# System Calls and ABI Reference

Syscall number definitions come from `PICONIX_SYSCALL_TABLE(X)` in `include/piconix_syscall.h`. The implementations are in `kernel/kernel.c` (`os_svc_dispatch()`), `userland/lib/syscalls.c`, and `userland/lib/unistd.h`. C programs normally use the wrappers; this is not the POSIX ABI.

## Calling convention

Execute `svc #0` with the syscall number in `r0` and the listed arguments, left to right, in `r1`, `r2`, `r3`. The result is returned in `r0`. APIs needing additional values use a structure pointer or are adapted by a user-space wrapper. Pointer arguments refer to user task memory.

Errors are normally returned directly as negative `OS_E*` values; `errno` is not used. `_exit()` never returns; the C `getcwd()` wrapper returns `buf` on success and `NULL` on failure. Numbers 11 and 15 are reserved.

## Complete syscall table

| No. | Kernel constant | C wrapper | r1 / r2 / r3 arguments | Return | Purpose |
|---:|---|---|---|---|---|
| 1 | `SYS_GETPID` | `getpid()` | — | PID | Return current PID |
| 2 | `SYS_KILL` | `kill(pid)` | `pid` | 0 / negative error | Terminate target task (not self) |
| 3 | `SYS_EXIT` | `_exit(status)` | `status` | no return | Terminate current task; does not return |
| 4 | `SYS_SLEEP` | `msleep(ms)` | `ms` | 0 | Sleep for milliseconds |
| 5 | `SYS_YIELD` | `yield()` | — | 0 | Yield CPU |
| 6 | `SYS_OPEN` | `open(path, flags)` | `path`, `flags` | FD / negative error | Open file/device |
| 7 | `SYS_CLOSE` | `close(fd)` | `fd` | 0 / negative error | Close FD |
| 8 | `SYS_READ` | `read(fd, buf, count)` | `fd`, `buf`, `count` | bytes read / negative error | Read from FD |
| 9 | `SYS_WRITE` | `write(fd, buf, count)` | `fd`, `buf`, `count` | bytes written / negative error | Write to FD |
| 10 | `SYS_IOCTL` | `ioctl(fd, cmd, value)` | `fd`, `cmd`, `value` | command-specific / negative error | Device control |
| 11 | `SYS_RESERVED_TASK_INFO` | `—` | — | `OS_ENOSYS` | Reserved |
| 12 | `SYS_SPAWN` | `spawn(program, argc, argv)` | `program`, `argc`, `argv` | child PID / negative error | Start registered program |
| 13 | `SYS_WAITPID` | `waitpid(pid, status, options)` | `pid`, `status`, `options` | child PID, 0, or negative error | Wait for child |
| 14 | `SYS_SET_FG` | `set_foreground(pid)` | `pid` | 0 / negative error | Set TTY foreground |
| 15 | `SYS_RESERVED_GETARG` | `—` | — | `OS_ENOSYS` | Reserved |
| 16 | `SYS_CONSOLE_READY` | `console_ready()` | — | 0 or 1 | Check console readiness |
| 17 | `SYS_UNLINK` | `unlink(path)` | `path` | 0 / negative error | Remove writable file |
| 18 | `SYS_DUP2` | `dup2(oldfd, newfd)` | `oldfd`, `newfd` | newfd / negative error | Duplicate FD to chosen number |
| 19 | `SYS_TTY_SPAWN` | `tty_spawn(tty, program)` | `tty`, `program` | child PID / negative error | Spawn task on TTY |
| 20 | `SYS_GET_TTY` | `get_tty()` | — | TTY ID | Get own TTY |
| 21 | `SYS_LSEEK` | `lseek(fd, offset, whence)` | `fd`, `offset`, `whence` | offset / negative error | Seek file position |
| 22 | `SYS_CHDIR` | `chdir(path)` | `path` | 0 / negative error | Change working directory |
| 23 | `SYS_GETCWD` | `getcwd(buf, size)` | `buf`, `size` | raw: 0 / negative error | Copy cwd into buffer |
| 24 | `SYS_FCNTL` | `fcntl(fd, cmd, value)` | `fd`, `cmd`, `value` | flags or 0 / negative error | Get/set FD flags |
| 25 | `SYS_SPAWN_TMP` | `spawn_tmp(path)` | `path` | child PID / negative error | Start PNX1 image in /tmp |
| 26 | `SYS_GETDENTS` | `getdents(fd, entry, size)` | `fd`, `entry`, `size` | 1, 0, or negative error | Read directory entry |
| 27 | `SYS_FLASH_ID` | `flash_id()` | — | ID value | Read flash identifier |
| 28 | `SYS_SOCKET` | `socket(domain, type, protocol)` | `domain`, `type`, `protocol` | FD / negative error | Create IPv4 TCP/UDP socket |
| 29 | `SYS_BIND` | `bind_udp(fd, port)` | `fd`, `port` | 0 / negative error | Bind UDP port |
| 30 | `SYS_SENDTO` | `sendto_udp(fd, packet)` | `fd`, `packet` | bytes sent / negative error | Send UDP datagram |
| 31 | `SYS_RECVFROM` | `recvfrom_udp(fd, packet)` | `fd`, `packet` | bytes received / negative error | Receive UDP datagram |
| 32 | `SYS_LISTEN_TCP` | `tcp_listen(fd, port)` | `fd`, `port` | 0 / negative error | Listen on TCP port |
| 33 | `SYS_CONNECT_TCP` | `tcp_connect(fd, endpoint)` | `fd`, `endpoint` | 0 / negative error | Initiate TCP connection |
| 34 | `SYS_TCP_STATUS` | `tcp_status(fd)` | `fd` | state / negative error | Check TCP connection state |
| 35 | `SYS_ACCEPT_TCP` | `tcp_accept(fd)` | `fd` | new FD / negative error | Accept TCP connection |
| 36 | `SYS_PIPE` | `pipe(fds)` | `fds` | 0 / negative error | Create read/write pipe FDs |
| 37 | `SYS_DUP` | `dup(oldfd)` | `oldfd` | new FD / negative error | Duplicate FD to free slot |
| 38 | `SYS_PROGRAM_ID` | `program_id(name)` | `name` | ID / negative error | Resolve program name to ID |
| 39 | `SYS_SHUTDOWN_TCP` | `shutdown(fd, SHUT_WR)` | `fd` | 0 / negative error | Half-close TCP sending side |

## Parameter meanings and important limitations

- `read(fd, buf, count)` / `write(fd, buf, count)`: `fd` identifies a descriptor, `buf` a user buffer, and `count` a byte limit. Zero bytes read may indicate EOF.
- `spawn(program, argc, argv)`: `program` is a registered ID; `argc` is 1–8; `argv` points to an array of strings. Total argument string storage is limited. Resolve names with `program_id(name)`.
- `waitpid(pid, status, options)`: `pid` is a child PID or `-1`; `status` is nullable; `options` is 0 or `WNOHANG` (1). The current implementation writes zero to `*status` on completion, unlike full POSIX exit-status reporting.
- `set_foreground(pid)`: `pid` is the PID of a child; 0 requests foreground restoration.
- `tty_spawn(tty, program)`: `tty` is 1 or 2; `program` is the registered shell/getty program ID (currently 2 or 4).
- `getcwd(buf, size)`: the raw syscall returns 0 on success; the C wrapper instead returns `buf` on success.
- `lseek(fd, offset, whence)`: regular `/tmp` files and `/dev/shm` support `SEEK_SET` (0), `SEEK_CUR` (1), and `SEEK_END` (2); `/dev/mem` supports absolute-address `SEEK_SET` only.
- `fcntl(fd, cmd, value)`: `cmd=1` corresponds to `F_GETFL`, and `cmd=2` to `F_SETFL`. `F_SETFL` can change `O_NONBLOCK` on TTYs (console, UART1, and Wi-Fi TTY when enabled) and TCP/UDP sockets. Pipe descriptors, regular files, and other devices are not supported. Use `ioctl()` for TTY modes.
- `pipe(fds)`: stores read FD in `fds[0]` and write FD in `fds[1]`. `dup()` and `dup2()` do not duplicate socket FDs.
- `socket(domain, type, protocol)`: only `AF_INET` (2), `SOCK_STREAM` (1) or `SOCK_DGRAM` (2), and protocol 0 are accepted.
- `bind_udp(fd, port)` and `tcp_listen(fd, port)`: port must be 1–65535. These operate on different socket types.
- `sendto_udp(fd, packet)` / `recvfrom_udp(fd, packet)`: `packet` is a `piconix_udp_packet_t *` containing data pointer, length, peer IPv4 address and port. Datagram size is bounded.
- `tcp_connect(fd, endpoint)`: `endpoint` is a `piconix_tcp_endpoint_t *`. Connection is asynchronous; check `tcp_status(fd)`. `tcp_accept(fd)` returns `OS_EAGAIN` if no connection is ready.
- `getdents(fd, entry, size)`: `entry` is `piconix_dirent_t *`. Returns 1 for an entry and 0 at directory end.
- `spawn_tmp(path)`: path names a PNX1-format Thumb image under `/tmp`.

## C APIs versus raw ABI

`bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, and `recvfrom()` are user-space BSD-style adapters, not separate SVC numbers. TCP `bind()` starts listening, `listen()` does not maintain a backlog, and `accept()` cannot return the peer address. See [NETWORK-PROGRAMMING.md](../development/apps/c/NETWORK-PROGRAMMING.md).

## Related references

- [LIBC.md](../development/apps/c/LIBC.md): user-level C library and functions
- [ASM.md](../development/apps/asm/ASM.md): SVC usage from assembly
- [PROGRAMMING.md](../development/apps/c/PROGRAMMING.md): application development
- [TTY.md](../literacy/TTY.md): TTY controls

`getdents()` now returns `device_id` (`0xffff` when unavailable).

The `device_id` field returned by `getdents()` identifies the directory entry: device ID for `/dev` and `/proc`, program ID for `/bin`, and zero-based RAM-file slot for `/tmp` and `/etc`. Directories or entries without an ID use `0xffff`. `ls -l` prints a `TYPE ID SIZE NAME` header, where `p` means program, `f` RAM file, `c` device, and `d` directory. RAM-file slot IDs are not persistent across deletion and recreation.
