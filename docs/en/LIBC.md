# PicoNIX C library and application APIs

[日本語](../ja/LIBC.md) | [C application development](PROGRAMMING.md) | [System calls](SYSCALLS.md)

PicoNIX provides a **small, purpose-built C library** for its user-space programs. It is **not** a complete ISO C library or POSIX libc. Applications use the headers and runtime under `userland/`; do not assume that arbitrary host-library functions are available. The implementation consists principally of `userland/libc.c` (string, conversion, and formatted I/O functions), `userland/syscalls.c` (system-call wrappers), and `userland/crt0.c` (startup and termination).

## Headers and available functions

The following entries match the headers and the implementation in this repository. Signatures are defined in the linked source headers; this table summarizes their roles rather than replacing the declarations.

| Header | Functions and definitions | Purpose |
|---|---|---|
| `stdio.h` | `putchar`, `getchar`, `puts`, `fgets` | Character and line input/output |
| `stdio.h` | `printf`, `sprintf`, `snprintf` | Limited formatted output |
| `stdio.h` | `FILE`, `stdin`, `stdout`, `stderr`, `EOF` | Minimal stream definitions; `FILE` holds an FD |
| `string.h` | `strlen`, `strcpy`, `strncpy`, `strcat`, `strcmp`, `strncmp`, `strchr`, `strstr` | String operations |
| `string.h` | `memcpy`, `memmove`, `memset`, `memcmp` | Memory operations |
| `stdlib.h` | `atoi`, `strtol`, `abs` | Integer conversion and absolute value |
| `unistd.h` | `read`, `write`, `open`, `close`, `unlink`, `lseek`, `getdents` | File and descriptor operations |
| `unistd.h` | `pipe`, `dup`, `dup2` | Pipes and descriptor duplication |
| `unistd.h` | `getpid`, `kill`, `waitpid`, `spawn`, `spawn_tmp`, `tty_spawn`, `program_id`, `set_foreground`, `_exit` | Tasks and process lifecycle |
| `unistd.h` | `msleep`, `yield`, `flash_id`, `get_tty`, `console_ready`, `chdir`, `getcwd`, `ioctl` | OS, directory, and terminal services |
| `unistd.h` | `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `sendto`, `recvfrom`, `tcp_status` | Small IPv4 TCP/UDP socket API |
| `unistd.h` | `bind_udp`, `sendto_udp`, `recvfrom_udp`, `tcp_listen`, `tcp_accept`, `tcp_connect` | PicoNIX-specific lower-level network interfaces |
| `fcntl.h` | `fcntl`, `O_*`, `F_GETFL`, `F_SETFL` | FD flags, including nonblocking mode |
| `piconix_dirent.h` | `piconix_dirent_t` | Fixed-size directory-entry structure |
| `piconix.h` | `OS_*` values, `piconix_udp_packet_t`, `piconix_tcp_endpoint_t` | Shared OS constants and structures |

`piconix.h` and `piconix_dirent.h` live in `include/`; `unistd.h` includes them as needed. Other application-specific headers include `trace.h` and `walk.h`. These are not general ISO C library headers.

## Frequently used functions: arguments and returns

| Function | Arguments | Return / important behavior |
|---|---|---|
| `printf(fmt, ...)` | Format string and values | Number of characters emitted; `-1` on write failure |
| `snprintf(dst, size, fmt, ...)` | Destination, buffer capacity, format and values | Number of characters that would have been written; NUL-terminates if `size > 0` and `dst` is valid |
| `puts(s)` | NUL-terminated string | Writes the string plus newline; `EOF` on failure |
| `putchar(c)` | Character value | Written byte value or `EOF` |
| `getchar()` | None | Input byte or `EOF` if one byte was not read |
| `fgets(s, size, stream)` | Destination, buffer capacity, stream | `s` on success; null pointer if no character was read; limited stream support |
| `strlen(s)` | NUL-terminated string | Number of bytes preceding NUL |
| `strtol(s, endptr, base)` | Numeric string, optional end pointer, base | Parsed `long`; overflow clamps to `LONG_MIN` or `LONG_MAX` (no `errno`) |
| `read(fd, buf, count)` | FD, buffer, byte capacity | Byte count, zero for EOF where applicable, or negative OS error |
| `write(fd, buf, count)` | FD, bytes, byte count | Byte count written or negative OS error |
| `open(path, flags)` | Path and PicoNIX `O_*` flags | FD or negative OS error |
| `msleep(milliseconds)` | Delay in milliseconds | OS service result (`int`) |
| `socket(domain, type, protocol)` | Address family, socket type, protocol | FD or negative OS error |

For **complete prototypes**, use the headers; for **system-call numbers, ABI registers and kernel behavior**, see [System calls](SYSCALLS.md). For the special network behavior (asynchronous `connect()`, `tcp_status()`, nonblocking `accept()`/`recvfrom()`), see [Network programming](NETWORK-PROGRAMMING.md).

For a complete example combining TTY `ioctl()` with FD `fcntl()` to poll keyboard input, see [Real-time keyboard input](PROGRAMMING.md#8-real-time-keyboard-input-noncanonical--o_nonblock) in the C programming guide.

## Important limitations

- Formatted output supports `%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%X`, `%%`, integer width/precision/padding and selected integer length modifiers. **Floating-point conversions are not supported**. Check `userland/libc.c` before using unfamiliar format features.
- Only the declared input/output stream functions are available; for example, **`fopen`, `fclose`, `fprintf`, `scanf`, `malloc`, `free`, `realloc`, and `exit` are not provided by these headers**.
- `sprintf()` has no destination length parameter: prefer `snprintf()` to avoid overflow.
- `fgets()` reads from `stream->fd`; its behavior is simpler than a fully buffered standard C stream.
- Most OS wrapper functions use **negative `OS_*` error codes directly**, without POSIX `errno`. Do not write code that assumes a host `errno` implementation.
- PicoNIX `O_*` and `F_*` flag values are **not host POSIX constants**. `lseek()` currently supports `SEEK_SET` only.
- Socket operations are a deliberately reduced IPv4 API, not full BSD sockets; `listen()` is a compatibility no-op after TCP `bind()` starts listening.

## Example

```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char *argv[]) {
    printf("PID: %d\n", getpid());
    if (argc > 1)
        printf("Argument: %s\n", argv[1]);
    msleep(100);
    return 0;
}
```

The application is built as a Flash-resident PIC image and copied into a RAM slot when launched. Returning from `main()` is handled by `crt0.c`. See [C application development](PROGRAMMING.md) for build and registration instructions.

## Implementation reference

- `userland/stdio.h`, `string.h`, `stdlib.h`, `unistd.h`, `fcntl.h`: public interfaces for applications
- `userland/libc.c`: locally implemented libc subset
- `userland/syscalls.c`: system call wrappers and socket compatibility functions
- `userland/crt0.c`: application entry and exit support
- `include/piconix.h`, `include/piconix_dirent.h`: OS definitions and shared structures
