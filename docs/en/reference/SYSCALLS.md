# System Calls and ABI Reference

*Technical reference*

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
| 40 | `SYS_SELECT` | `select(nfds, readfds, writefds, NULL, timeout)` | `nfds`, `piconix_select_request_t *`, — | ready FD count / negative error | Wait for readiness among up to 16 FDs |

## Parameter meanings and important limitations

- `read(fd, buf, count)` / `write(fd, buf, count)`: `fd` identifies a descriptor, `buf` a user buffer, and `count` a byte limit. Zero bytes read may indicate EOF. For TTYs, a nonblocking `read()` also returns 0 when input is not ready, in either canonical or noncanonical mode. TTY `write()` can be partial; a full USB CDC transmit queue can return 0, while Wi-Fi TTY can return `OS_EAGAIN` or `OS_EIO`. See the [TTY guide](../guides/TTY.md).
- `spawn(program, argc, argv)`: `program` is a registered ID; `argc` is 1–8; `argv` points to an array of strings. Total argument string storage is limited. Resolve names with `program_id(name)`.
- `waitpid(pid, status, options)`: `pid` is a child PID or `-1`; `status` is nullable; `options` is 0 or `WNOHANG` (1). The current implementation writes zero to `*status` on completion, unlike full POSIX exit-status reporting.
- `set_foreground(pid)`: `pid` is the PID of a child; 0 requests foreground restoration.
- `tty_spawn(tty, program)`: `tty` is 1 or 2; `program` is the registered shell/getty program ID (currently 2 or 4).
- `getcwd(buf, size)`: the raw syscall returns 0 on success; the C wrapper instead returns `buf` on success.
- `lseek(fd, offset, whence)`: regular `/tmp` files, `/dev/shm`, and the read-only `/proc/sym/kernel` support `SEEK_SET` (0), `SEEK_CUR` (1), and `SEEK_END` (2). Symbol table offsets are byte offsets within the exposed text; seeking beyond EOF is rejected. `/dev/mem` supports absolute-address `SEEK_SET` only. Other `/proc` files, including `/proc/sym/userland/*`, remain non-seekable.
- `fcntl(fd, cmd, value)`: `cmd=1` corresponds to `F_GETFL`, and `cmd=2` to `F_SETFL`. `F_SETFL` can change `O_NONBLOCK` on TTYs (console, UART1, and Wi-Fi TTY when enabled) and TCP/UDP sockets. Pipe descriptors, regular files, and other devices are not supported. Use `ioctl()` for TTY modes.
- `pipe(fds)`: stores read FD in `fds[0]` and write FD in `fds[1]`. `dup()` and `dup2()` do not duplicate socket FDs.
- `socket(domain, type, protocol)`: only `AF_INET` (2), `SOCK_STREAM` (1) or `SOCK_DGRAM` (2), and protocol 0 are accepted.
- `bind_udp(fd, port)` and `tcp_listen(fd, port)`: port must be 1–65535. These operate on different socket types.
- `sendto_udp(fd, packet)` / `recvfrom_udp(fd, packet)`: `packet` is a `piconix_udp_packet_t *` containing data pointer, length, peer IPv4 address and port. Datagram size is bounded.
- `tcp_connect(fd, endpoint)`: `endpoint` is a `piconix_tcp_endpoint_t *`. Connection is asynchronous; check `tcp_status(fd)`. `tcp_accept(fd)` returns `OS_EAGAIN` if no connection is ready.
- `getdents(fd, entry, size)`: `entry` is `piconix_dirent_t *`. Returns 1 for an entry and 0 at directory end.
- `spawn_tmp(path)`: path names a PNX1-format Thumb image under `/tmp`.

- TCP/UDP socket `read()`/`write()` are polled independently of `O_NONBLOCK`; unavailable I/O returns `OS_EAGAIN`. `fcntl()` changes the flag but not the socket waiting behavior. An undersized UDP receive buffer returns `OS_EINVAL` without consuming the datagram. TCP `read()` returns 0 after a normal peer close and drain.

## Boundary conditions for `open()`, `unlink()`, and `getdents()`

- `open()` accepts the access modes `O_RDONLY`, `O_WRONLY`, `O_RDWR` and supported combinations of `O_CREAT`, `O_TRUNC`, `O_APPEND`, and `O_NONBLOCK`. Invalid combinations return `OS_EINVAL`. At `open()` time, `O_NONBLOCK` is accepted only for the console and UART1; supported TTYs (including Wi-Fi TTY) and sockets can change it later with `fcntl(F_SETFL)`.
- Opening a missing file under `/tmp` or `/etc` without `O_CREAT` returns `OS_ENOENT`. Creating a file when all eleven RAM-file slots are occupied returns `OS_ENOSPC`; running out of FDs returns `OS_EMFILE`. RAM filenames are at most 27 ASCII bytes (31 bytes including the internal `tmp/` or `etc/` prefix).
- `O_TRUNC` truncates RAM files only when opened in a writable mode. `O_APPEND` targets the end of the file on every write. A write at the 2-KiB boundary may be partial; when no bytes fit, a nonempty write returns `OS_ENOSPC`.
- `unlink()` returns `OS_EBUSY` when any task still has that RAM file open, or `OS_ENOENT` if it is missing.
- `getdents()` works on directory FDs only. It returns `1` per entry and `0` at the end. A buffer smaller than `sizeof(piconix_dirent_t)` returns `OS_EINVAL`; a non-directory FD returns `OS_EBADF`.
- `lseek()` is restricted to supported FD types. RAM files and `/dev/shm` can seek past their current data end, but not beyond 2 KiB (`OS_EINVAL`). Unsupported FDs also return `OS_EINVAL`.

## Process and FD errors and resource ownership

- `spawn(program, argc, argv)` returns `OS_ENOENT` for an unregistered ID, `OS_EINVAL` for invalid `argc` (outside 1–8), pointers, or argument storage (including more than 128 bytes of strings), and `OS_EAGAIN` when all six user slots are occupied. Only standard FDs 0–2 are inherited from the parent; other FDs are not inherited.
- `waitpid(pid, status, options)` accepts a child PID or `-1`; `options` must be 0 or `WNOHANG`. Invalid arguments return `OS_EINVAL`. If neither a matching running child nor an unreaped exit notification exists, it returns `OS_ESRCH`. With `WNOHANG`, a still-running child causes a return of 0. Exited tasks immediately release their task slots; one PID can be reaped from each notification. At most six exit notifications are retained, and if full, the first array entry is overwritten, so an unreaped notification may be lost. `status` is always set to 0 regardless of the reason for termination.
- `kill(pid)` terminates the target and returns 0 on success. An unknown PID, the current task, and the idle task return `OS_ESRCH`. This is not general POSIX signal delivery.
- `pipe(fds)` returns 0 on success; invalid output storage returns `OS_EINVAL`, fewer than two available FDs returns `OS_EMFILE`, and both pipe slots already in use returns `OS_EAGAIN`. Reading an empty pipe or writing a full pipe waits; an empty pipe with no writers returns EOF (0), and writing with no readers returns `OS_EIO`.
- `dup(oldfd)` allocates a free FD; `dup2(oldfd, newfd)` replaces a chosen FD. Invalid FDs return `OS_EBADF`; `dup()` with no free FD returns `OS_EMFILE`. A socket source, or a socket destination in `dup2()`, returns `OS_EINVAL`. `dup2(fd, fd)` returns the valid FD unchanged. Pipe endpoint references are updated on duplication, inheritance, `close()`, and task exit.

## C APIs versus raw ABI

`bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, and `recvfrom()` are user-space BSD-style adapters, not separate SVC numbers. TCP `bind()` starts listening, `listen()` does not maintain a backlog, and `accept()` cannot return the peer address. See [NETWORK-PROGRAMMING.md](../development/apps/c/NETWORK-PROGRAMMING.md).

## Related references

- [LIBC.md](../development/apps/c/LIBC.md): user-level C library and functions
- [ASM.md](../development/apps/asm/ASM.md): SVC usage from assembly
- [PROGRAMMING.md](../development/apps/c/PROGRAMMING.md): application development
- [TTY.md](../guides/TTY.md): TTY controls

`getdents()` now returns `device_id` (`0xffff` when unavailable).

The `device_id` field returned by `getdents()` identifies the directory entry: device ID for `/dev` and `/proc`, program ID for `/bin`, and zero-based RAM-file slot for `/tmp` and `/etc`. Directories or entries without an ID use `0xffff`. `ls -l` prints a `TYPE ID SIZE NAME` header, where `p` means program, `f` RAM file, `c` device, and `d` directory. RAM-file slot IDs are not persistent across deletion and recreation.


The `spawn_foreground()` and `spawn_tmp_foreground()` C wrappers are not extra syscall numbers: they request an atomic foreground TTY handoff as part of `SYS_SPAWN` (argument count flag `0x100`) or `SYS_SPAWN_TMP` (second argument 1). Ordinary `spawn()` remains suitable for background tasks. This avoids the scheduling race between `spawn()` and a separate `set_foreground()` call.

---

**Related links:** [Reference index](README.md) | [Learning roadmap](../ROADMAP.md) | [日本語](../../ja/reference/SYSCALLS.md)

## I/O multiplexing with `select()`

`select()` checks readiness for TCP/UDP sockets, TTYs and pipes in the kernel, blocking the task when necessary. A `NULL` timeout waits indefinitely and `{0,0}` polls immediately. FDs are limited to 0–15; `exceptfds` is unsupported. `fd_set` is a 16-bit mask. Existing blocking `accept()`, `recv()`, and `recvfrom()` remain available.

```c
fd_set rd;
FD_ZERO(&rd);
FD_SET(server, &rd);
int n = select(server + 1, &rd, NULL, NULL, NULL);
if (n > 0 && FD_ISSET(server, &rd)) {
    int client = accept(server, NULL, NULL);
}
```

TCP write readiness reflects available send-buffer space, not a guarantee that the full write succeeds. Always check the result of the subsequent I/O call. Monitoring unsupported FD types, such as regular RAM files, currently returns `OS_EINVAL`. Timeout accuracy depends on scheduler ticks.
