# Developing C applications for PicoNIX

[日本語](../ja/PROGRAMMING.md) | [Overview](README.md) | [Internals](INTERNAL.md)

This guide takes you from modifying an existing program to **adding and running a new command**. PicoNIX applications are not ordinary Linux executables: they are PIC images embedded in Flash and copied into RAM when launched.

## 1. Try an existing program

Follow [Setup](SETUP.md) to build and flash the UF2. Then try:

```text
piconix# hello
Hello, PicoNIX!
piconix# hello Pico
Hello, Pico!
piconix# ls /bin
```

To experiment with an existing source, change the `msleep(500)` value in `userland/blink.c`, rebuild and verify the behavior. LED behavior depends on the board and pin configuration.

## 2. A minimal C program

`userland/hello.c` is a simple output example:

```c
#include "stdio.h"

int main(int argc, char *argv[]) {
    if (argc > 1)
        printf("Hello, %s!\n", argv[1]);
    else
        puts("Hello, PicoNIX!");
    return 0;
}
```

You can also print with just `write()` from `unistd.h`:

```c
#include "unistd.h"

int main(int argc, char *argv[]) {
    const char msg[] = "hello\n";
    write(1, msg, sizeof(msg) - 1);
    return 0;
}
```

Use PicoNIX's headers and minimal libc, rather than linking directly against the host libc or Pico SDK. When `main()` returns, the shared startup routine in `userland/crt0.c` performs termination.

## 3. Add a new command

1. Create a C source under `userland/`; use `hello.c` as a starting point.
2. Add the application build, PIC checks and Flash image embedding to `CMakeLists.txt`, following an existing entry.
3. Add image start/end symbol declarations and a `register_image()` entry to `kernel/main.c`. Assign an **unused program ID** (capacity 32; ID 0 is reserved).
4. Rebuild and confirm the PIC and slot-size checks pass.
5. Flash the UF2 and run your new command from the Shell.

You do **not** need to add command-specific code to the Shell. The kernel registers names and IDs, which the Shell discovers through `/bin`. Files placed in `/tmp` cannot be loaded as executable programs.

## 4. RAM layout and build constraints

| Item | Requirement |
|---|---|
| Entry | `main(int argc, char *argv[])` (through `crt0.c`) |
| Slots | Five 32-KiB task slots |
| Stack | Last 4 KiB of each slot reserved |
| Application | Build as PIC and embed in Flash |
| Execution | Copy into a free RAM slot |
| Kernel services | System calls through SVC |

`userland/slot.ld` keeps code/data from overlapping the reserved stack area. The build compares images linked at different RAM addresses and rejects invalid PIC configurations such as problematic GOT or unresolved references. An ordinary host libc cannot be linked as-is.

## 5. Arguments and return values

External commands receive arguments through `argc` and `argv`. The Shell splits on spaces; it does not group quoted strings. Up to eight strings, including the command name, can be passed. Their combined storage is at most 128 bytes including NUL terminators. `argv[argc]` is `NULL`.

Programs can also be started with `spawn(program_id, argc, argv)`. Starting one fails when all task slots are occupied. `waitpid()` implements simple child waiting, not full POSIX exit-status handling.

## 6. Commonly used APIs

| Header | Examples |
|---|---|
| `unistd.h` | `read`, `write`, `open`, `close`, `msleep`, `spawn`, `waitpid` |
| `fcntl.h` | `O_RDONLY`, `O_WRONLY`, `O_CREAT`, `O_NONBLOCK`, `F_GETFL`, `F_SETFL` |
| `stdio.h` | `printf`, `snprintf`, `puts`, `putchar`, `getchar`, `fgets` |
| `string.h` | Basic string and memory operations |
| `stdlib.h` | `atoi`, `strtol`, `abs` |

`printf` supports `%s`, `%c`, `%d`, `%i`, `%u`, `%x`, `%X` and `%%`, but not width, precision or floating-point formats. `msleep()` takes milliseconds. These are deliberately limited APIs, not complete POSIX implementations.

## 7. Files and redirection

The Shell's `<`, `>` and `>>` operators work by setting the standard input/output descriptors:

```text
piconix# echo hello > /tmp/message
piconix# cat /tmp/message
hello
piconix# hexdump /tmp/message
```

Applications can use `read(0, ...)` and `write(1, ...)` without knowing whether input/output has been redirected. `/tmp` files live in RAM and disappear on reboot. `lseek()` supports `SEEK_SET` for ordinary `/tmp` files and `/dev/mem`. Avoid using regular-file flags such as `O_CREAT` and `O_TRUNC` for devices.

## 8. TTY and nonblocking input

```c
#include "unistd.h"
#include "fcntl.h"

int flags = fcntl(0, F_GETFL, 0);
if (flags >= 0)
    fcntl(0, F_SETFL, flags | O_NONBLOCK);
/* For character input, also disable canonical mode using ioctl. */
```

`O_NONBLOCK` controls a file descriptor, independently of canonical and echo terminal modes. The game in `userland/mb.c` demonstrates temporarily switching the input mode. See [Terminal settings and interactive input](TTY.md) for restoring settings and additional cautions.

## 9. Debugging and verification

```sh
python3 tools/test_audit.py
python3 tools/test_ram_pipeline.py
```

These checks alone do not guarantee a complete firmware build or hardware behavior. Build the firmware with the ARM toolchain and Pico SDK. Watch for unresolved `__aeabi_*` symbols, slot-size overruns, failed PIC checks and mismatched system-call numbers.

For runtime diagnostics, use `ps`, `cat /proc/tasks`, `cat /proc/memory` and `trace on/off/show`. See [Internals](INTERNAL.md) and [Tracing](TRACE.md).

## 10. Development checklist

- [ ] Program ID does not conflict with another registration
- [ ] `main(argc, argv)` is defined
- [ ] Only PicoNIX user APIs are used
- [ ] PIC and size checks pass
- [ ] The five-slot limit is considered
- [ ] Arguments, I/O and termination have been tested on hardware

**Next:** Read [Internals](INTERNAL.md) for the OS architecture or [Devices and wiring](DEVICES.md) for hardware-specific access.

## Network programming

Pico W user applications can access TCP/UDP sockets. See [Socket API and network applications](NETWORK-PROGRAMMING.md).
