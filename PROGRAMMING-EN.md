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
4. Register the program in `src/main.c` and `CMakeLists.txt`; Shell discovers external programs via `/bin`.
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

All RAM applications and `crt0.c` now use the user API; only `syscalls.c` directly issues SVC instructions. `msleep()` takes milliseconds, unlike POSIX. The API is not a complete POSIX implementation.


## Minimal user-space C library

`ram_apps/libc.c` supplies a compact subset of standard C functions through
`stdio.h` (`printf`, `snprintf`, `sprintf`, `puts`, `putchar`, `getchar`, `fgets`),
`string.h` (strings and memory), and `stdlib.h` (`atoi`, `strtol`, `abs`).

Formatted output supports only `%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%X`, and `%%`.
Width, precision, floating point, and `scanf` are not supported.
`fgets` uses a minimal `FILE` representation, including `stdin`, and retains the newline.
`puts` appends a newline; explicitly emit `\r\n` if a console needs CRLF.
The library uses user `read`/`write` syscalls, not the kernel's Newlib.
Unused functions are garbage-collected, and all sixteen application images are
PIC-checked at two RAM origins during the host-side test.

## Example external applications

`ram_apps/hello.c` demonstrates `printf()` and `puts()` (`hello` or `hello NAME`). `ram_apps/hexdump.c` reads a file using `read()` and prints hexadecimal and ASCII columns (`hexdump /tmp/FILE`). Without a filename, it reads standard input. Both are PIC user applications embedded in firmware Flash and loaded into task slots.

## /dev/mem and hexdump

`hexdump /dev/mem 0x20018000 64` displays 64 bytes from the first user slot; `hexdump /dev/mem 0x10000000 64` inspects flash. Regular files also accept an offset and size, e.g. `hexdump /tmp/test 0 32`. `lseek(fd, offset, SEEK_SET)` supports `/dev/mem` and regular `/tmp` files. Memory is read-only and exposes kernel contents; use for teaching and debugging only.

### Kernel tracing (16 KiB)

`trace on` clears and starts the binary ring; `trace off` stops recording; `trace show` stops recording and prints the retained events. `/dev/trace` provides read-only, fixed 32-byte event records. Trace controls use device-specific `ioctl()` commands; decoding stays in the `trace.c` user application.

```text
trace on
hello
trace off
trace show
```

Events include task switches and SVC entry/exit. Timestamps use SysTick milliseconds since boot.


## Command arguments and /bin

External applications use `int main(int argc, char *argv[])`. `spawn(program_id, argc, argv)` accepts up to eight arguments including the command name, with a combined limit of 128 bytes including NUL terminators. Over-limit input is rejected. `argv[argc]` is `NULL`. The shell splits on whitespace; quote parsing is not supported.

`/bin` is a read-only virtual directory enumerating registered PIC applications, not executable files on disk. The Shell command `ls /bin` also displays builtin names after `---- sh built-in commands ----`. The kernel-provided `/bin` remains unchanged. Use `ls /bin`; both `hello` and `/bin/hello` work. Images remain in Flash and are loaded into a RAM slot on spawn.

```
hexdump /dev/mem 0x20018000 64
trace on
hello world
trace off
trace show
```

`trace show` displays Time(ms), PID, Command, Event and Detail columns. Command is resolved from the program ID captured at event time via `/bin`, so exited tasks retain their names. For switches, PID and Command refer to the destination task. Binary records remain 16 bytes.

Trace details: [`TRACE-EN.md`](TRACE-EN.md) / [`TRACE-JP.md`](TRACE-JP.md)


## waitpid / Shell built-ins

`waitpid(pid, status, 0)` blocks until the child exits; `WNOHANG` is nonblocking. Child task slots are released immediately, and up to five PID-only completion notifications are retained. Exit status is not retained. Orphan exit notifications are discarded. `ls /bin` lists built-ins after `---- sh built-in commands ----`.


## External ps, cat, watch and interrupt counters

`ps` and `cat` are standalone PIC applications (`/bin/ps`, `/bin/cat`), no longer Shell builtins. `watch` is a standalone `/bin/watch` application.

```text
ps
cat /proc/tasks
watch ps
watch 1 ps
watch 1 cat /proc/interrupts
watch 2 cat /proc/memory
```

`watch [seconds] command [args...]` repeatedly executes an external program. The interval is 1–60 seconds, default 1 second. The terminal is cleared using ANSI escapes. `watch` and its child occupy one task slot each. `ls` and `echo` are external commands and can run under `watch`. Ctrl+C only terminates the terminal's foreground PID, not a process group; a child may remain briefly if interrupted while running. Use short-lived commands.

`SysTick` in `/proc/interrupts` counts actual 1-ms interrupts. `uart1` counts entries into the UART1 IRQ handler. UART0-console builds report UART0 IRQs as `console`. USB CDC builds omit `console` because USB IRQs are not counted. `PendSV` and `SVC` count exception handler entries.


## External shell commands and working directories

`pwd`, `ls`, `rm`, `echo`, `kill`, and `tty` are separate `/bin` PIC applications. The `pid` command was removed; `getpid()` remains available. Shell built-ins are `help`, `cd`, `exit`, `logout`, and `getty`.

The kernel maintains one working directory per task, inherited through `spawn()`. `chdir()` changes the calling task's directory; `getcwd()` returns it. `open()` and `unlink()` resolve relative paths. After `cd /tmp`, `pwd`, `ls`, `cat name`, and `rm name` use that directory.

`ls /bin` lists external applications, followed by the Shell built-ins under their own heading. `watch 1 ls /tmp` and `watch 1 pwd` are now supported.

### ADC devices (RP2040)

`/dev/adc/0`, `/dev/adc/1`, `/dev/adc/2` expose the 12-bit inputs on GPIO26–28. They are read-only. Opening a device captures one raw value (0–4095), formatted as decimal plus newline; further reads reach EOF. `watch 1 cat /dev/adc/0` resamples. GPIO26–28 are initialized for ADC and should not simultaneously be used as ordinary GPIO.

`/dev/adc/temp` exposes the RP2040 internal temperature sensor. `cat /dev/adc/temp` reports an approximate Celsius value with one decimal place; `watch 1 cat /dev/adc/temp` refreshes it. The sensor is uncalibrated and assumes a 3.3 V ADC reference.

### PWM and reserved GPIO pins (2026-10-27)

GP18 and GP19 are dedicated PWM outputs, sharing a default 1 kHz frequency, with initial duties 50% and 0%. `/dev/pwm/1/freq` accepts 8–100000 Hz, and `/dev/pwm/1/a` and `/dev/pwm/1/b` accept 0–100 percent via decimal text reads/writes. GPIO listings and access exclude GP18, GP19 and ADC-reserved GP26–28.
