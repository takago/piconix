# PicoNIX internals

*Technical reference*

This guide explains **how PicoNIX works**, from boot and task switching to system calls and I/O. For application implementation details, see [C application development](../development/apps/c/PROGRAMMING.md). For physical pin assignments and connections, see [pin assignments](hardware/PINS.md) and [wiring](hardware/WIRING.md); see [devices](devices/README.md) for `/dev` interfaces.

## 1. Architecture at a glance

PicoNIX is a small UNIX-like OS running on the RP2040. The kernel provides scheduling, tasks, virtual files and I/O. The Shell and commands run as separate RAM applications.

```text
Flash
 ├─ Kernel + Pico SDK
 └─ Embedded PIC application images (/bin)
                   | spawn
                   v
RAM
 ├─ Kernel region
 └─ Six 24-KiB task slots
     └─ Shell / external commands (copied at launch)
           | SVC
           v
         Kernel --- /dev, /proc, /tmp
```

PicoNIX has no Linux ELF loader, MMU-based process isolation, dynamic linking or general POSIX signals. Its goal is to demonstrate core UNIX ideas in a small implementation.

## 2. Memory layout and PIC applications

| Start address | Region | Capacity |
|---|---|---:|
| `0x20000000` | Kernel RAM region | 56 KiB |
| `0x2000E000` | Task slot 1 | 24 KiB |
| `0x20014000` | Task slot 2 | 24 KiB |
| `0x2001A000` | Task slot 3 | 24 KiB |
| `0x20020000` | Task slot 4 | 24 KiB |
| `0x20026000` | Task slot 5 | 24 KiB |
| `0x2002C000` | Task slot 6 | 24 KiB |
| `0x20032000` | Shared memory (SHM) | 2 KiB |
| `0x20032800` | RAM-file payloads (11 × 2 KiB) | 22 KiB |
| `0x20038000` | Dedicated trace ring | 16 KiB |
| `0x2003C000` | Network reserve | 16 KiB |

The final 4 KiB of each task slot is reserved for its stack, leaving at most 20 KiB for application image and static data. Six task slots are available; idle processing is managed separately.

Applications are built as position-independent code (PIC) and embedded in Flash. At launch, an application image is copied into a free RAM slot and given an initial stack. `userland/lib/slot.ld` defines the layout. During the build, images linked at different addresses are compared to check position independence. See [C application development](../development/apps/c/PROGRAMMING.md).

## 3. Boot and task management

1. `kernel/main.c` invokes `os_init()` to initialize the kernel.
2. It registers Flash-embedded applications with program names and IDs.
3. It starts the Shell (program ID 2) and hands control to the scheduler.
4. SysTick and PendSV then drive task switching; low-level exception and context handling are in `kernel/context.S`.

Task states are `UNUSED`, `READY`, `SLEEPING`, `BLOCKED`, and `DEAD`. `spawn()` launches a registered program in a new slot. `waitpid()` waits for a child, or returns without blocking when `WNOHANG` is specified. Exit codes (0–255) are retained in completion notices separately from reusable task slots and returned through `waitpid()`.

`ps` and `/proc/tasks` show the parent PID in `PPID` and per-task dispatch counts in the trailing `DISPATCH` column. The PID 0 `[idle]` row and cumulative `CPU_TICKS` column are also shown.

## 4. System-call boundary

RAM applications do not call kernel functions directly. The wrappers in `userland/lib/syscalls.c` execute an SVC instruction. Register `r0` holds the call number, `r1`–`r3` hold arguments, and `r0` receives the result. Dispatch is implemented in `kernel/kernel.c`.

| Area | Main APIs |
|---|---|
| Tasks | `getpid()`, `spawn()`, `waitpid()`, `kill()`, `yield()`, `msleep()` |
| Files | `open()`, `read()`, `write()`, `close()`, `lseek()`, `unlink()`, `dup2()` |
| Directories | `chdir()`, `getcwd()` |
| TTY and control | `get_tty()`, `ioctl()`, `fcntl()`, `set_foreground()` |

The kernel and user-side system-call numbers must agree. System-call number 15 is reserved and must not be allocated. Startup arguments reach `main(argc, argv)` through the initial stack and `crt0.c`.

For the complete numbered ABI, see [System Call Reference](SYSCALLS.md).

## 5. Shell and processes

The kernel keeps each task's working directory, file descriptors and associated TTY. A child inherits the relevant state. The Shell parses commands and starts registered `/bin` programs; only a few commands, such as `cd`, are built in.

```text
piconix# cd /tmp
piconix# echo hello > note
piconix# cat note
hello
piconix# ps
```

The Shell implements `<`, `>` and `>>` by setting standard descriptors 0, 1 and 2 before starting a child. `dup2()` is simplified and does not implement POSIX shared file offsets. Pipelines of up to three commands are supported. Quoted argument grouping and `2>` are unsupported.

A child inherits only standard FDs (0–2). Pipe endpoints gain a reference when inherited or duplicated, and lose one on `close()` or task termination. A `getty` spawned for another TTY releases its inherited standard FDs before attaching the new terminal. `dup()`/`dup2()` of TCP/UDP sockets is not supported.

With `command &`, the Shell returns without waiting for the external command. `Ctrl+C` directly terminates the registered foreground tasks of the respective terminal (up to three pipeline stages); it does not deliver POSIX signals to process groups.

On task exit, the kernel releases FDs and sockets and removes the completed PID from the foreground task registry. If a pipeline stage exits early, a surviving stage inherits foreground terminal ownership and responsibility for restoring saved TTY settings. `waitpid()` completion notices are kept separately from reusable task slots (including its exit code from 0–255). PID 0 is reserved for idle; regular PIDs cycle through 1–255. Allocation skips active tasks and exit notices not yet collected by `waitpid()`. When resuming a blocked terminal read, pipe I/O, or `waitpid()`, the kernel fills in the saved SVC return value and clears temporary wait state and borrowed user pointers before marking the task READY.

## 6. TTY and I/O

PicoNIX provides two independent USB CDC virtual TTYs (`/dev/ttyV0` and `/dev/ttyV1`, switched with Ctrl+T) plus a separate UART1 terminal. For example, `getty /dev/ttyS1 sh &` starts another Shell. Each terminal tracks its owning Shell and foreground PID; `Ctrl+C` acts on that terminal's foreground task.

Use `ioctl()` to set **terminal input modes** (canonical/noncanonical and echo) and `fcntl()` to set **per-FD nonblocking flags**. These settings are independent. See [Terminal settings and interactive input](commands/TTY.md) for examples. This is not a full POSIX `termios` implementation: `VMIN` and `VTIME` are not provided.

## 7. Virtual files and devices

| Path | Purpose |
|---|---|
| `/bin` | Read-only list of Flash-embedded registered programs |
| `/tmp` | Small RAM files, lost on reboot |
| `/dev/ttyV0`, `/dev/ttyS1` | Terminal I/O |
| `/dev/gpio/N`, `/dev/adc/*`, `/dev/pwm/*` | GPIO, ADC and PWM |
| `/dev/mem` | Memory read/write in permitted ranges |
| `/dev/trace` | Binary tracing records |
| `/proc/tasks`, `/proc/uptime`, `/proc/interrupts`, `/proc/memory`, `/proc/cpu` | Kernel status |

See [devices](devices/README.md) for device paths, and [pin assignments](hardware/PINS.md) for the corresponding GPIO roles. Each task has its own descriptors, and the same `read()` and `write()` APIs operate on files and devices.

Internally, `open()` and `getdents()` share the same parser for `/dev/gpio/N`, `/dev/gpio/N.dir`, and `/dev/gpio/N.pull`; `kernel/pins.h` determines which pins are available. Path normalization checks each user-string address before reading it. Unsupported device IDs are not dispatched to GPIO and unsupported read/write operations return an error. Transfers through `/dev/i2c/1` use `ioctl()`, not `read()` or `write()`.

## 8. Tracing and debugging

The kernel uses a dedicated ring buffer at `0x20038000`–`0x2003BFFF`, holding 512 records of 32 bytes each (16 KiB total). It records task switches and SVC entry/exit events. The `trace` command turns the records into readable text. To prevent competing writes from SysTick, UART1 and SVC, each record and the ring counters are updated in a short interrupt-masked section.

```text
trace on -s
hello
trace off
trace show
```

The standard build enables function instrumentation, but event capture starts OFF at boot. `trace on -s` records only syscall entry/exit, while ordinary `trace on` also captures selected kernel functions, TIMER and SWITCH. `trace on -a` adds selected non-SVC functions.

`/dev/trace` provides the binary records. See [Tracing](commands/TRACE.md) for event formats and interpretation.

## 9. Implementation sources

| File | Responsibility |
|---|---|
| `kernel/main.c` | Startup and application registration |
| `kernel/kernel.c` | Tasks, SVC, descriptors, `/proc`, `/tmp`, TTY |
| `kernel/context.S` | Exceptions and context switching |
| `kernel/gpio.c`, `adc.c`, `pwm.c`, `fb.c` | Device drivers |
| `kernel/pins.h` | Exposed pins and initial assignments |
| `userland/apps/shell.c` | Command parsing and redirection |
| `userland/lib/syscalls.c`, `crt0.c` | User-side SVC wrappers and startup |
| `userland/lib/slot.ld`, `CMakeLists.txt` | RAM layout, images and PIC validation |

**Next:** [C application development](../development/apps/c/PROGRAMMING.md) for adding a command, [devices](devices/README.md) for device interfaces, [wiring](hardware/WIRING.md) for physical connections, and [Terminal settings and interactive input](commands/TTY.md) for TTY details.

Pico W uses the 16-KiB network reserve for its 10-KiB lwIP heap and 4-KiB idle task stack; see [Memory layout](MEMORY-LAYOUT.md).

---

**Related links:** [Reference index](README.md) | [日本語](../../ja/reference/INTERNAL.md) | [Usage guides](commands/README.md)
