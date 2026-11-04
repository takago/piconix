# PicoNIX C library and application APIs

*Development guide*

PicoNIX provides a **small, purpose-built C library** for its user-space programs. It is **not** a complete ISO C library or POSIX libc. Applications use the headers and runtime under `userland/`; do not assume that arbitrary host-library functions are available. The implementation consists principally of `userland/lib/libc.c` (string, conversion, and formatted I/O functions), `userland/lib/syscalls.c` (system-call wrappers), and `userland/lib/crt0.c` (startup and termination).

## Headers and available functions

The following entries match the headers and the implementation in this repository. Signatures are defined in the linked source headers; this table summarizes their roles rather than replacing the declarations.

| Header | Functions and definitions | Purpose |
|---|---|---|
| `stdio.h` | `putchar`, `getchar`, `puts`, `fgets` | Character and line input/output |
| `stdio.h` | `printf`, `sprintf`, `snprintf`, `scanf`, `sscanf`, `fprintf`, `fscanf` | Limited formatted input/output |
| `stdio.h` | `FILE`, `stdin`, `stdout`, `stderr`, `EOF` | Small streams with an FD and shared 64-byte buffer |
| `stdio.h` | `fopen`, `fclose`, `fread`, `fwrite`, `fflush`, `setvbuf`, `feof`, `ferror` | Basic file streams |
| `string.h` | `strlen`, `strcpy`, `strncpy`, `strcat`, `strcmp`, `strncmp`, `strchr`, `strstr` | String operations |
| `string.h` | `memcpy`, `memmove`, `memset`, `memcmp`, `memchr` | Memory operations |
| `stdlib.h` | `atoi`, `strtol`, `strtoul`, `abs` | Integer conversion and absolute value |
| `unistd.h` | `read`, `write`, `open`, `close`, `unlink`, `lseek`, `getdents` | File and descriptor operations |
| `unistd.h` | `pipe`, `dup`, `dup2` | Pipes and descriptor duplication |
| `unistd.h` | `getpid`, `kill`, `waitpid`, `spawn`, `spawn_tmp`, `tty_spawn`, `program_id`, `set_foreground`, `_exit` | Tasks and process lifecycle |
| `unistd.h` | `msleep`, `yield`, `flash_id`, `get_tty`, `console_ready`, `chdir`, `getcwd`, `ioctl` | OS, directory, and terminal services |
| `unistd.h` | `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `sendto`, `recvfrom`, `tcp_status`, `shutdown` | Small IPv4 TCP/UDP socket API |
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
| `scanf(fmt, ...)` | Format and destination pointers | Number of assigned fields; `EOF` for input end before conversion |
| `sscanf(src, fmt, ...)` | Input string, format, destinations | Same limited conversions as `scanf()` on a string |
| `strtoul(s, endptr, base)` | Text, optional end pointer, base | Unsigned integer; overflow clamps to `ULONG_MAX` (no `errno`) |
| `strlen(s)` | NUL-terminated string | Number of bytes preceding NUL |
| `strtol(s, endptr, base)` | Numeric string, optional end pointer, base | Parsed `long`; overflow clamps to `LONG_MIN` or `LONG_MAX` (no `errno`) |
| `read(fd, buf, count)` | FD, buffer, byte capacity | Byte count, zero for EOF where applicable, or negative OS error |
| `write(fd, buf, count)` | FD, bytes, byte count | Byte count written or negative OS error |
| `open(path, flags)` | Path and PicoNIX `O_*` flags | FD or negative OS error |
| `msleep(milliseconds)` | Delay in milliseconds | OS service result (`int`) |
| `socket(domain, type, protocol)` | Address family, socket type, protocol | FD or negative OS error |

For **complete prototypes**, use the headers; for **system-call numbers, ABI registers and kernel behavior**, see [System calls](../../../reference/SYSCALLS.md). For the special network behavior (asynchronous `connect()`, `tcp_status()`, nonblocking `accept()`/`recvfrom()`), see [Network programming](NETWORK-PROGRAMMING.md).

For a complete example combining TTY `ioctl()` with FD `fcntl()` to poll keyboard input, see [Section 8, “Real-time keyboard input,” in the C programming guide](PROGRAMMING.md).


`shutdown(fd, SHUT_WR)` half-closes only the TCP sending side; it is not a general implementation of every POSIX `shutdown()` mode.

## Important limitations

- Formatted output supports `%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%X`, `%%`, integer width/precision/padding and selected integer length modifiers. **Floating-point conversions are not supported**. Check `userland/lib/libc.c` before using unfamiliar format features.
- This is not a full C library: **`malloc`, `free`, `realloc`, and `exit` are not provided**.
- `sprintf()` has no destination length parameter: prefer `snprintf()` to avoid overflow.
- `fgets()`, `scanf()`, `fscanf()`, and `fread()` share the `FILE` input buffer. Bytes already prefetched are not visible to direct `read()` calls.
- Most OS wrapper functions use **negative `OS_*` error codes directly**, without POSIX `errno`. Do not write code that assumes a host `errno` implementation.
- PicoNIX `O_*` and `F_*` flag values are **not host POSIX constants**. `lseek()` supports `SEEK_SET`, `SEEK_CUR`, and `SEEK_END` for `/tmp` and `/dev/shm`, but only `SEEK_SET` for `/dev/mem`.
- Socket operations are a deliberately reduced IPv4 API, not full BSD sockets; `listen()` is a compatibility no-op after TCP `bind()` starts listening.

## Small 64-byte streams

Each `FILE` contains an FD and **one 64-byte buffer**, used for either reading or writing; it does not allocate separate 64-byte input and output buffers. No heap is used. Up to four `fopen()` streams can be open simultaneously, in addition to the three standard streams.

| Stream | Buffering |
|---|---|
| `stdin`, `fopen("...", "r")` | 64-byte input buffer |
| `stdout` | Line buffered (flush on newline) |
| `stderr` | Unbuffered |
| `fopen("...", "w")`, `fopen("...", "a")` | 64-byte output buffer |

`fopen()` supports `r`, `w`, `a` and their `b` variants; update modes containing `+` are not supported. `fclose()` flushes output and closes the FD, while `fflush(NULL)` flushes all output streams. Returning normally from `main()` flushes pending output; `_exit()` or forced termination does not guarantee this.

`setvbuf()` allows `_IOFBF`, `_IOLBF`, and `_IONBF` using the internal buffer only (pass NULL as its second argument). Common full-screen helpers switch to unbuffered output to prevent delayed display. Call `fflush(stdout)` before a direct `read()` when a prompt has no newline.

```c
#include "stdio.h"

int main(void) {
    FILE *fp = fopen("/tmp/example.txt", "w");
    if (!fp) return 1;
    fprintf(fp, "Hello, PicoNIX!\n");
    if (fclose(fp) == EOF) return 1;

    fp = fopen("/tmp/example.txt", "r");
    if (!fp) return 1;
    char line[64];
    while (fgets(line, sizeof(line), fp)) printf("%s", line);
    fclose(fp);
    return 0;
}
```

`fread()` and `fwrite()` return **item counts, not byte counts**. `feof()` reports EOF and `ferror()` reports an I/O error. Mixing buffered `FILE` operations with direct `read()` or `write()` on the same FD can disrupt the expected data position and order.

## Formatted input (`scanf()` and `sscanf()`)

`scanf()` reads standard input; `sscanf()` reads a string. Supported conversions are `%d`, `%i` (automatic base), `%u`, `%x`, `%X`, `%s`, `%c`, and `%%`. Field widths and assignment suppression (`%*d`) are supported. **Floating-point input, `%n`, and length modifiers such as `%ld` are not implemented.** This is a deliberately limited subset, not a full ISO C `scanf` implementation.

```c
int first, second;
if (sscanf("3 4", "%d %d", &first, &second) == 2)
    printf("%d and %d\n", first, second);
```

For standard input, use `scanf("%d", &first)`. Repeated numeric conversions consume successive tokens, so `3 4` can be read with two calls. `getchar()` and `fgets(stdin)` share the input buffer with `scanf()`. Do not mix these functions with direct `read(0, ...)` when unread lookahead may be pending.

**Always specify a field width for `%s`**, leaving room for the terminating NUL byte: use `%7s` for `char name[8]`. `%c` does not terminate a string. When a conversion cannot parse input, the offending character remains unread and the function normally returns zero assignments. Repeating that call without consuming invalid input can loop forever.

For teaching and predictable error handling, reading a line with `fgets()` and parsing it with `sscanf()` is often simpler.

## Learning standard input with `omikuji -i`

`omikuji` runs once with a 10-second default, while `omikuji 3` runs once with three seconds. With `omikuji -i`, `scanf("%d", &seconds)` repeatedly reads integers from standard input. `echo "3 4" | omikuji -i` runs twice and ends at EOF. Blank lines are skipped by `scanf()`; invalid tokens are discarded through `getchar()` until a separator. **A failed conversion leaves unread input behind and may cause an infinite loop if the program does not consume it.**

The countdown uses `fflush(stdout)` before sleeping, so text does not remain in the 64-byte output buffer. Without `-i`, no standard input is read, and `omikuji &` uses the default 10 seconds.

## `fgets()` edge cases

In the current `userland/lib/libc.c`, `fgets(s, size, stream)` returns NULL when `size <= 0` or an argument is NULL. When `size == 1`, it stores only the terminating NUL, returns `s`, and consumes no input. When `size > 1` and no initial character can be read, it returns NULL. Account for terminal settings and the EOF return convention.

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

- `userland/lib/stdio.h`, `string.h`, `stdlib.h`, `unistd.h`, `fcntl.h`: public interfaces for applications
- `userland/lib/libc.c`: locally implemented libc subset
- `userland/lib/syscalls.c`: system call wrappers and socket compatibility functions
- `userland/lib/crt0.c`: application entry and exit support
- `include/piconix.h`, `include/piconix_dirent.h`: OS definitions and shared structures

## Directory-entry device IDs

The current `piconix_dirent_t` includes a `uint16_t device_id` field, and `ls -l` displays entry IDs. See the [system call reference](../../../reference/SYSCALLS.md) for the meanings of these IDs and the `0xffff` sentinel. If the ABI changes, rebuild both the kernel and user programs.

## Starting interactive child programs

Interactive child programs can use `spawn_foreground(program, argc, argv)` or `spawn_tmp_foreground(path)` when the child needs TTY access immediately. These functions make the spawn and foreground ownership change atomic; after `waitpid()` the parent calls `set_foreground(0)` to reclaim the terminal. Use plain `spawn()` for background jobs.

---

**Related links:** [Development guide index](../../README.md) | [Learning roadmap](../../../ROADMAP.md) | [日本語](../../../../ja/development/apps/c/LIBC.md)
