# Developing C applications for PicoNIX

[日本語](../ja/PROGRAMMING.md) | [Documentation guide](DOCS.md) | [C library](LIBC.md)

This guide shows how to add a **new C command** to PicoNIX, build it on a host computer, and run it on a board. It documents the build mechanism in the current source tree. For the toolchain and board setup, first see [Setup](SETUP.md).

PicoNIX applications are **not Linux executables**. The host compiler produces small position-independent (PIC) images. The firmware stores them in Flash; launching a command copies its image into an available RAM task slot. The kernel itself runs code from Flash using XIP.

## 1. Before writing a new command

Once PicoNIX boots, try the existing example:

```text
piconix# hello
Hello, PicoNIX!
piconix# hello Pico
Hello, Pico!
```

The source is `userland/hello.c`. Each application normally defines `main(int argc, char *argv[])`, and PicoNIX's shared `userland/crt0.c` handles termination after `main` returns. Applications use PicoNIX's headers and small C library, rather than a host libc or Pico SDK library directly.

## 2. Create `greet.c`

Create **`userland/greet.c`** with the following complete source:

```c
#include "stdio.h"

int main(int argc, char *argv[])
{
    const char *name = (argc > 1) ? argv[1] : "PicoNIX";
    printf("Hello, %s!\n", name);
    return 0;
}
```

The first argument, `argv[0]`, is the command name; an optional name is `argv[1]`. A normal return from `main` ends the application. The example uses `printf` from PicoNIX's `stdio.h`; consult [C library and APIs](LIBC.md) for the available functions and format limitations.

## 3. Add the application to the firmware

There are **two required changes** in the current build system.

### 3.1 Add the source name to `CMakeLists.txt`

Find `foreach(APP IN ITEMS ...)` near the top of `CMakeLists.txt` and append `greet` **without the `.c` suffix**:

```cmake
foreach(APP IN ITEMS pins fbctl pwmctl kitesys shell blink omikuji python hello hexdump trace ps cat grep watch pwd ls rm echo kill tty stty mb reset lsof pong gpio fbed asm help edit mem tree find nc httpd tcp_echod udp_echod greet)
```

The line above shows the current `CMakeLists.txt` with `greet` appended. Keep all existing application names; if the list changes in a newer version, append `greet` to that version instead. The common loop automatically compiles `userland/greet.c` with `crt0.c`, `syscalls.c`, and `libc.c`, links the RAM image, validates its PIC behavior and size, and embeds the binary in firmware Flash. No separate custom build rule is needed.

### 3.2 Register the embedded image in `kernel/main.c`

Add a symbol declaration beside the other application images:

```c
extern const unsigned char piconix_greet_image_start[], piconix_greet_image_end[];
```

Add a call alongside the other `register_image(...)` calls in `main()`:

```c
register_image(39, "greet", piconix_greet_image_start, piconix_greet_image_end);
```

**39 is unused in the source snapshot used for this example.** Before using the example in a newer version, check the existing IDs and choose an unused one. PicoNIX currently supports registry IDs **1–64** (0 is reserved). The `greet` name is what will appear in `/bin` and be resolved by the shell. You do not need to modify the shell to recognize the command. Files in `/tmp` are not dynamically loaded as native C executables.

## 4. Build and run

From the repository root, after completing the SDK/toolchain setup:

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

For Pico W, replace `pico` with `pico_w`. Follow [Setup](SETUP.md) if your installation paths differ, and for the resulting UF2 location and board flashing procedure. A successful firmware build includes PIC and task-slot checks. After flashing, try:

```text
piconix# greet
Hello, PicoNIX!
piconix# greet Alice
Hello, Alice!
piconix# ls /bin
```

If `greet` is missing, confirm both the CMake list entry and `register_image` call, then rebuild and reflash. A host-only test does not install new commands into a running device.

## 5. Arguments, file descriptors, and input/output

The shell passes `argc` and `argv` to external programs. It uses space-separated tokens without quote grouping. It passes up to **eight strings including the command name**, with a total argument-string storage limit of **128 bytes including NUL terminators**; `argv[argc]` is `NULL`.

Standard descriptors are 0 (input), 1 (output), and 2 (error). The same calls work with shell redirection and pipes:

```c
#include "unistd.h"

int main(int argc, char *argv[])
{
    const char message[] = "hello\n";
    write(1, message, sizeof(message) - 1);
    return 0;
}
```

For example, `greet Alice > /tmp/message` redirects output to a RAM file. File and descriptor support is intentionally smaller than POSIX; see [Filesystem](FILESYSTEM.md) and [Pipes](PIPE.md). Device access is documented in [Devices](DEVICES.md).

## 6. APIs and execution constraints

| Topic | Current behavior |
|---|---|
| Entry point | `main(argc, argv)` via `userland/crt0.c` |
| RAM task slots | 6 slots × 24 KiB |
| Stack reserve | Last 4 KiB in each slot; image must fit the remaining space |
| Executable image | PIC, embedded in firmware Flash, copied into RAM at launch |
| Kernel interface | C wrappers using SVC system calls |
| Process creation | `spawn(program_id, argc, argv)`; fails when no free slot is available |
| Waiting | `waitpid()` is a small PicoNIX subset, not full POSIX semantics |

Use `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, and `fcntl.h` as supplied in `userland/`. For their actual functions, argument types, error returns, and supported formats, see [LIBC.md](LIBC.md). For low-level ABI details see [SYSCALLS.md](SYSCALLS.md). The kernel interface returns PicoNIX error values rather than standard `errno` semantics.

`O_NONBLOCK` (set with `fcntl()`) does not turn canonical TTY input into character-at-a-time input; for interactive games, see [TTY](TTY.md) and `userland/mb.c`. Network applications have additional constraints; see [Network programming](NETWORK-PROGRAMMING.md).

## 7. Diagnose problems

| Symptom | Check |
|---|---|
| Compiler error | Includes, function declarations, and PicoNIX's actual header interfaces |
| Undefined symbol (including `__aeabi_*`) | Unsupported dependencies or compiler helper/linker configuration |
| PIC or image-size check fails | Non-position-independent code, unsupported relocation, or the RAM image/stack limit |
| Command missing from `/bin` | CMake entry, image symbol declaration, registration, and whether the new UF2 was flashed |
| Spawn fails at runtime | Available task slots and program registration |
| TTY appears to behave incorrectly | Canonical/echo mode and restoration; see [TTY](TTY.md) |

Host-side regression checks:

```sh
python3 tools/run_tests.py
python3 tools/run_tests.py --all  # includes toolchain-dependent tests
```

The complete firmware build and real hardware checks are still necessary; source-level tests alone do not prove a board will run correctly. Use `ps`, `cat /proc/tasks`, `trace on`, `trace off`, and `trace show` when diagnosing runtime behavior. [Tracing](TRACE.md) explains syscall and context-switch events; [Memory inspection](MEM.md) explains read-only RAM/Flash inspection.

## 8. Real-time keyboard input (noncanonical + O_NONBLOCK)

This example reads individual keys without waiting for a newline and keeps running when no key is pressed. Interactive tools and games configure **TTY input processing** separately from **FD blocking behavior**.

- `ioctl(0, OS_TTY_SET_CANON, 0)` disables line buffering so characters can be read individually.
- `ioctl(0, OS_TTY_SET_ECHO, 0)` disables automatic input echo.
- `fcntl(0, F_SETFL, flags | O_NONBLOCK)` prevents `read()` from waiting for input.

You can save this example as `userland/keypoll.c`. It uses PicoNIX-specific TTY `ioctl` commands, not POSIX `termios`.

```c
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

int main(void)
{
    int canon = ioctl(0, OS_TTY_GET_CANON, 0);
    int echo  = ioctl(0, OS_TTY_GET_ECHO, 0);
    int flags = fcntl(0, F_GETFL, 0);
    int result = 1;

    if (canon < 0 || echo < 0 || flags < 0) {
        puts("A TTY is required.");
        return 1;
    }

    if (ioctl(0, OS_TTY_SET_CANON, 0) < 0 ||
        ioctl(0, OS_TTY_SET_ECHO, 0) < 0 ||
        fcntl(0, F_SETFL, flags | O_NONBLOCK) < 0) {
        puts("Cannot configure TTY.");
        goto restore;
    }

    puts("Counter runs without input. Press q to quit.");
    for (unsigned count = 0;; ++count) {
        char key;
        int n = read(0, &key, 1);
        if (n == 1 && (key == 'q' || key == 'Q')) {
            result = 0;
            break;
        }
        if (n < 0) { /* Actual input error. */
            puts("TTY read error.");
            break;
        }
        /* n == 0: no key available in nonblocking mode. */
        if (count % 10 == 0)
            printf("Count: %u\n", count);
        msleep(100);
    }

restore:
    fcntl(0, F_SETFL, flags);
    ioctl(0, OS_TTY_SET_ECHO, echo);
    ioctl(0, OS_TTY_SET_CANON, canon);
    return result;
}
```

**Important PicoNIX behavior:** a nonblocking TTY `read()` returns **0** when no character is ready; do not assume Linux-style `EAGAIN` behavior. In noncanonical mode, Ctrl+D (0x04) is delivered as an ordinary input byte rather than EOF; Ctrl+D has different behavior in canonical mode. Negative results indicate errors. `msleep(100)` prevents busy polling when no input is available.

The example saves and restores canonical mode, echo, and FD flags, including when configuration fails partway through. **Forced termination such as Ctrl+C or `kill` bypasses the C cleanup path.** The kernel's foreground-task restoration mechanism handles the TTY settings in that situation. FD flags are task-local and are explicitly restored on ordinary exit.

Like the earlier `greet` example, this application must be added to CMake and registered in the kernel (substitute `keypoll` in the preceding steps). No prior `stty raw` command is needed. See [TTY](TTY.md) for mode semantics, and `userland/mb.c` and `userland/pong.c` for real game implementations.

## 9. Where to go next

- [C library](LIBC.md): functions available to applications and their limitations
- [System calls](SYSCALLS.md): syscall numbers, arguments, return values, and ABI
- [Assembly programming](ASM.md) and [Python](PYTHON.md): programs created inside PicoNIX (a different workflow from host-compiled C)
- [Architecture](ARCHITECTURE.md) and [Internals](INTERNAL.md): PIC loading, scheduling, and kernel implementation
- [Network programming](NETWORK-PROGRAMMING.md): TCP/UDP application development on Pico W
