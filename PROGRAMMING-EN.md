# Developing C applications

[日本語](PROGRAMMING-JP.md) | [Back to overview](README-EN.md)

PicoNIX applications are not ordinary Linux executables. They are **small standalone programs built as position-independent code (PIC)**. Start by modifying Blink to understand the workflow.

## 1. First exercise: Blink

Change the millisecond values passed to `SYS_SLEEP` in `ram_apps/blink.c`. For example, use `250` instead of `500` for faster blinking. Build using [Setup](SETUP-EN.md), flash the UF2, and run `blink`.

## 2. RAM application rules

| Item | Requirement |
|---|---|
| Entry point | `main(argc, argv)` |
| RAM addresses | Five dynamically allocated slots from `0x20018000`, spaced 32 KiB apart |
| Size | 32 KiB each (last 4 KiB reserved for stack) |
| Kernel interface | SVC (`r0` number, `r1`–`r3` arguments, return in `r0`) |
| Avoid | Direct kernel linking and ordinary libc/SDK dependencies |

See `ram_apps/blink.c` for a working minimal example. Keep the SVC convention and numbers consistent with `src/kernel.c`.

## 3. Adding a command

1. Create a C source under `ram_apps/` using the existing application pattern.
2. Add its PIC image build and embedding in `CMakeLists.txt`.
3. Register the program in `src/main.c`.
4. Add command dispatch in `ram_apps/shell.c`.
5. Up to five application tasks can run; a spawn fails when no slot is free.

The flash-embedded image design does not support loading executable files from `/tmp`.

## 4. Files and standard I/O

Use `SYS_OPEN`, `SYS_READ`, `SYS_WRITE`, and `SYS_CLOSE` to access devices and `/tmp` files. Shell handles `<`, `>`, and `>>` and passes descriptors to launched tasks. `dup2` does not provide POSIX shared offsets. Do not pass regular-file `O_CREAT`/`O_TRUNC` flags when opening devices.

## 5. Checks and pitfalls

```sh
python3 tools/test_audit.py
python3 tools/test_ram_pipeline.py
```

These are host checks. The host RAM-image pipeline checks Shell and Blink; **building with ARM GCC and Pico SDK is still required for all application images**. Watch for unresolved helpers such as `__aeabi_*`, slot overflows, and mismatched syscall numbers.

For job control and the absence of UNIX signals, see [Internals](INTERNAL-EN.md).

## Common startup (crt0)

Every RAM application links `ram_apps/crt0.c`. The kernel sets `r0=argc` and `r1=argv` in the initial task frame; `app_entry()` in crt0 invokes `main(argc, argv)` and issues `SYS_EXIT` when main returns. `SYS_GETARG` is removed; syscall number 15 remains reserved.

## User application API (stage 6: addition only)

`ram_apps/unistd.h` and `ram_apps/fcntl.h` expose user-facing declarations and flags; `ram_apps/syscalls.c` implements minimal SVC wrappers. Every RAM app links `syscalls.o`; unused functions are removed by section garbage collection.

```c
#include "unistd.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {
    const char message[] = "hello\n";
    write(1, message, sizeof(message) - 1);
    return 0;
}
```

All RAM applications and `crt0.c` now use the user API; only `syscalls.c` directly issues SVC instructions. `sleep()` takes milliseconds, unlike POSIX. The API is not a complete POSIX implementation.


## Minimal user-space C library

`ram_apps/libc.c` supplies a compact subset of standard C functions through
`stdio.h` (`printf`, `snprintf`, `sprintf`, `puts`, `putchar`, `getchar`, `fgets`),
`string.h` (strings and memory), and `stdlib.h` (`atoi`, `strtol`, `abs`).

Formatted output supports only `%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%X`, and `%%`.
Width, precision, floating point, and `scanf` are not supported.
`fgets` uses a minimal `FILE` representation, including `stdin`, and retains the newline.
`puts` appends a newline; explicitly emit `\r\n` if a console needs CRLF.
The library uses user `read`/`write` syscalls, not the kernel's Newlib.
Unused functions are garbage-collected, and all five application images are
PIC-checked at two RAM origins during the host-side test.

## Example external applications

`ram_apps/hello.c` demonstrates `printf()` and `puts()` (`hello` or `hello NAME`). `ram_apps/hexdump.c` reads a file using `read()` and prints hexadecimal and ASCII columns (`hexdump /tmp/FILE`). Without a filename, it reads standard input. Both are PIC user applications embedded in firmware Flash and loaded into task slots.

## /dev/mem and hexdump

`hexdump /dev/mem 0x20018000 64` displays 64 bytes from the first user slot; `hexdump /dev/mem 0x10000000 64` inspects flash. Regular files also accept an offset and size, e.g. `hexdump /tmp/test 0 32`. `lseek(fd, offset, SEEK_SET)` supports `/dev/mem` and regular `/tmp` files. Memory is read-only and exposes kernel contents; use for teaching and debugging only.

### Kernel tracing (16 KiB)

`trace on` clears and starts the binary ring; `trace off` stops recording; `trace show` stops recording and prints the retained events. `/dev/trace` provides read-only, fixed 16-byte event records. Trace controls use device-specific `ioctl()` commands; decoding stays in the `trace.c` user application.

```text
trace on
hello
trace off
trace show
```

Events include task switches and SVC entry/exit. Timestamps use SysTick milliseconds since boot.

`spawn(program, value, argument)` forwards `argument` as `argv[1]` for every app (one string, up to 30 bytes). It does not yet implement general multi-argument parsing. The `value` parameter retains the legacy numeric Omikuji behavior.
