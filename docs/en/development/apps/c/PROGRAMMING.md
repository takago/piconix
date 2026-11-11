# Developing C applications for PicoNIX

*Development guide*

## Requirements

The example assumes C functions, arrays and command-line arguments. Source changes on the build PC require a new firmware build and UF2 flash.


[日本語](../../../../ja/development/apps/c/PROGRAMMING.md) | [Documentation guide](../../../README.md) | [C library](LIBC.md)

This guide shows how to add a **new C command** to PicoNIX, build it on a host computer, and run it on a board. It documents the build mechanism in the current source tree. For the toolchain and board setup, see [Setup](../../../setup/SETUP.md).

PicoNIX applications are **not Linux executables**. The host compiler produces small position-independent (PIC) images. The firmware stores them in Flash; launching a command copies its image into an available RAM task slot. The kernel itself runs code from Flash using XIP.

## 1. Existing command implementation

The existing `hello` command produces:

```text
piconix# hello
Hello, PicoNIX!
piconix# hello Pico
Hello, Pico!
```

The source is `userland/apps/hello.c`. Each application normally defines `main(int argc, char *argv[])`, and PicoNIX's shared `userland/lib/crt0.c` handles termination after `main` returns. Applications use PicoNIX's headers and small C library, rather than a host libc or Pico SDK library directly.

## 2. Create `greet.c`

Create **`userland/apps/greet.c`** with the following complete source:

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
foreach(APP IN ITEMS fd net ip tcpdump ping pins i2cctl bh1750 fbctl pwmctl kitesys shell blink omikuji python hello hexdump trace ps cat cp grep watch pwd ls rm echo kill tty stty mb reset lsof pong reversi gpio fbed asm cc help edit vi dd mem tree find nc httpd tcp_echod udp_echod mcpd scroll pty_test greet)
```

The line above shows the current application list with `greet` appended. Keep all existing application names when extending the list. The common loop automatically compiles `userland/apps/greet.c` with `crt0.c`, `syscalls.c`, and `libc.c`, links the RAM image, validates its PIC behavior and size, and embeds the binary in firmware Flash. No separate custom build rule is needed.

### 3.2 Register the embedded image in `kernel/main.c`

Add a symbol declaration beside the other application images:

```c
extern const unsigned char piconix_greet_image_start[], piconix_greet_image_end[];
```

Add a call alongside the other `register_image(...)` calls in `main()`:

```c
register_image(52, "greet", piconix_greet_image_start, piconix_greet_image_end);
```

**52 is available in the current `kernel/main.c`.** Choose an unused program ID when adding an application. PicoNIX currently supports registry IDs **1–64** (0 is reserved). The `greet` name is what will appear in `/bin` and be resolved by the shell. You do not need to modify the shell to recognize the command. Files in `/tmp` are not dynamically loaded as native C executables.

## 4. Build and run

From the repository root, after completing the SDK/toolchain setup:

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

For Pico W, replace `pico` with `pico_w`. Follow [Setup](../../../setup/SETUP.md) if your installation paths differ, and for the resulting UF2 location and board flashing procedure. A successful firmware build includes PIC and task-slot checks. After flashing, try:

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

For example, `greet Alice > /tmp/message` redirects output to a RAM file. File and descriptor support is intentionally smaller than POSIX; see [Filesystem](../../../reference/FILESYSTEM.md) and [Pipes](../../../reference/commands/PIPE.md). Device access is documented in [Devices](../../../reference/devices/README.md).

## 6. APIs and execution constraints

| Topic | Current behavior |
|---|---|
| Entry point | `main(argc, argv)` via `userland/lib/crt0.c` |
| RAM task slots | 6 slots × 24 KiB |
| Stack reserve | Last 4 KiB in each slot; image must fit the remaining space |
| Executable image | PIC, embedded in firmware Flash, copied into RAM at launch |
| Kernel interface | C wrappers using SVC system calls |
| Process creation | `spawn(program_id, argc, argv)`; fails when no free slot is available |
| Waiting | `waitpid()` is a small PicoNIX subset, not full POSIX semantics |

Use `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`, and `fcntl.h` as supplied in `userland/lib/`. For their actual functions, argument types, error returns, and supported formats, see [LIBC.md](LIBC.md). For low-level ABI details see [SYSCALLS.md](../../../reference/SYSCALLS.md). The kernel interface returns PicoNIX error values rather than standard `errno` semantics.

`O_NONBLOCK` (set with `fcntl()`) does not turn canonical TTY input into character-at-a-time input; for interactive games, see [TTY](../../../reference/commands/TTY.md) and `userland/apps/mb.c`. Network applications have additional constraints; see [Network programming](NETWORK-PROGRAMMING.md).

### 6.1. PicoNIX-specific PIC and RAM constraints

Unlike an ordinary Linux executable, a PicoNIX C application must use the **same binary image in any available RAM task slot**. The build links the program at two different addresses and checks that the resulting binaries match and require no unsupported relocations or GOT. There is no dynamic linker to relocate absolute references at load time.

#### Static pointer initializers

The following may compile but fail the PicoNIX PIC check:

```c
/* Avoid on PicoNIX: stores an address in static data */
static const char *message = "Hello";
```

Assign the address during execution instead:

```c
static const char *message;  /* initially zero */

int main(void)
{
    message = "Hello";
    puts(message);
    return 0;
}
```

If no pointer is necessary, `static const char message[] = "Hello";` defines the array itself and is another option. Watch for static initializers containing addresses of functions or other objects. This is **not a general C-language prohibition**: it follows from PicoNIX's current loader not applying runtime relocations.

#### Image size, stack and optimization

- Each user slot is 24 KiB, including a reserved 4 KiB stack. Code, static data and BSS must fit in the remaining region.
- Avoid large local arrays, deep recursion and unbounded buffers. Successful linking does not prove stack use is safe.
- **All user applications are compiled with `-Oz` and `-fPIC`** using the ARM compiler selected by CMake. Match these options in size/PIC checks.
- `ld.lld: task image overlaps reserved stack` indicates that the program image exceeds the non-stack portion. Reduce code or static storage rather than increasing the slot limit.
- `PIC image changes with link address` indicates an address-dependent binary. Inspect static pointer initializers, absolute references and relocation requirements.

A static pointer initializer such as `static const char *notice = "";` can fail the PIC check. Initializing the pointer at runtime avoids the absolute relocation; **successful C compilation alone does not establish PIC validity**.

## 7. Diagnose problems

| Symptom | Check |
|---|---|
| Compiler error | Includes, function declarations, and PicoNIX's actual header interfaces |
| Undefined symbol (including `__aeabi_*`) | Unsupported dependencies or compiler helper/linker configuration |
| PIC or image-size check fails | Non-position-independent code, unsupported relocation, or the RAM image/stack limit |
| Command missing from `/bin` | CMake entry, image symbol declaration, registration, and whether the new UF2 was flashed |
| Spawn fails at runtime | Available task slots and program registration |
| TTY appears to behave incorrectly | Canonical/echo mode and restoration; see [TTY](../../../reference/commands/TTY.md) |

Host-side regression checks:

```sh
python3 tools/run_tests.py
python3 tools/run_tests.py --all  # includes toolchain-dependent tests
```

The complete firmware build and real hardware checks are still necessary; source-level tests alone do not prove a board will run correctly. Use `ps`, `cat /proc/tasks`, `trace on`, `trace off`, and `trace show` when diagnosing runtime behavior. [Tracing](../../../reference/commands/TRACE.md) explains syscall and context-switch events; [Memory inspection](../../../reference/commands/MEM.md) explains read-only RAM/Flash inspection.

## 8. Real-time keyboard input (noncanonical + O_NONBLOCK)

This example reads individual keys without waiting for a newline and keeps running when no key is pressed. Interactive tools and games configure **TTY input processing** separately from **FD blocking behavior**.

- `ioctl(0, OS_TTY_SET_CANON, 0)` disables line buffering so characters can be read individually.
- `ioctl(0, OS_TTY_SET_ECHO, 0)` disables automatic input echo.
- `fcntl(0, F_SETFL, flags | O_NONBLOCK)` prevents `read()` from waiting for input.

You can save this example as `userland/apps/keypoll.c`. It uses PicoNIX-specific TTY `ioctl` commands, not POSIX `termios`.

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

Like the earlier `greet` example, this application must be added to CMake and registered in the kernel (substitute `keypoll` in the preceding steps). No prior `stty raw` command is needed. See [TTY](../../../reference/commands/TTY.md) for mode semantics, and `userland/apps/mb.c` and `userland/apps/pong.c` for real game implementations.

## 9. Where to go next

- [C library](LIBC.md): functions available to applications and their limitations
- [System calls](../../../reference/SYSCALLS.md): syscall numbers, arguments, return values, and ABI
- [Assembly programming](../asm/ASM.md) and [Python](../python/PYTHON.md): programs created inside PicoNIX (a different workflow from host-compiled C)
- [Architecture](../../../reference/ARCHITECTURE.md) and [Internals](../../../reference/INTERNAL.md): PIC loading, scheduling, and kernel implementation
- [Network programming](NETWORK-PROGRAMMING.md): TCP/UDP application development on Pico W

---

**Related links:** [Development guide index](../../README.md)
