# PicoNIX internals

[日本語](../ja/INTERNAL.md) | [Overview](README.md) | [C application development](PROGRAMMING.md)

This guide explains **how PicoNIX works**, from boot and task switching to system calls and I/O. For step-by-step application development, see [C application development](PROGRAMMING.md). For pin assignments, see [Devices and wiring](DEVICES.md).

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
 └─ Five 32-KiB task slots
     └─ Shell / external commands (copied at launch)
           | SVC
           v
         Kernel --- /dev, /proc, /tmp
```

PicoNIX has no Linux ELF loader, MMU-based process isolation, dynamic linking or general POSIX signals. Its goal is to demonstrate core UNIX ideas in a small implementation.

## 2. Memory layout and PIC applications

| Start address | Region | Capacity |
|---|---|---:|
| `0x20000000` | Kernel RAM region | 96 KiB |
| `0x20018000` | Task slot 1 | 32 KiB |
| `0x20020000` | Task slot 2 | 32 KiB |
| `0x20028000` | Task slot 3 | 32 KiB |
| `0x20030000` | Task slot 4 | 32 KiB |
| `0x20038000` | Task slot 5 | 32 KiB |

The final 4 KiB of each task slot is reserved for its stack. Program code and static data must fit in the remainder. Five task slots are available concurrently; idle processing is managed separately.

Applications are built as position-independent code (PIC) and embedded in Flash. At launch, an application image is copied into a free RAM slot and given an initial stack. `userland/slot.ld` defines the layout. During the build, images linked at different addresses are compared to check position independence. See [C application development](PROGRAMMING.md).

## 3. Boot and task management

1. `kernel/main.c` invokes `os_init()` to initialize the kernel.
2. It registers 24 Flash-embedded applications with program names and IDs.
3. It starts the Shell (program ID 2) and hands control to the scheduler.
4. SysTick and PendSV then drive task switching; low-level exception and context handling are in `kernel/context.S`.

Task states are `UNUSED`, `READY`, `SLEEPING`, `BLOCKED`, and `DEAD`. `spawn()` launches a registered program in a new slot. `waitpid()` waits for a child, or returns without blocking when `WNOHANG` is specified. Exit handling is simplified: exit status values are not retained.

## 4. System-call boundary

RAM applications do not call kernel functions directly. The wrappers in `userland/syscalls.c` execute an SVC instruction. Register `r0` holds the call number, `r1`–`r3` hold arguments, and `r0` receives the result. Dispatch is implemented in `kernel/kernel.c`.

| Area | Main APIs |
|---|---|
| Tasks | `getpid()`, `spawn()`, `waitpid()`, `kill()`, `yield()`, `msleep()` |
| Files | `open()`, `read()`, `write()`, `close()`, `lseek()`, `unlink()`, `dup2()` |
| Directories | `chdir()`, `getcwd()` |
| TTY and control | `get_tty()`, `ioctl()`, `fcntl()`, `set_foreground()` |

The kernel and user-side system-call numbers must agree. Number 15 is reserved for the former `GETARG` call and must not be reused. Startup arguments reach `main(argc, argv)` through the initial stack and `crt0.c`.

## 5. Shell and processes

The kernel keeps each task's working directory, file descriptors and associated TTY. A child inherits the relevant state. The Shell parses commands and starts registered `/bin` programs; only a few commands, such as `cd`, are built in.

```text
piconix# cd /tmp
piconix# echo hello > note
piconix# cat note
hello
piconix# ps
```

The Shell implements `<`, `>` and `>>` by setting standard descriptors 0, 1 and 2 before starting a child. `dup2()` is simplified and does not implement POSIX shared file offsets. Pipes, quoted argument grouping and `2>` are unsupported.

With `command &`, the Shell returns without waiting for the external command. `Ctrl+C` directly terminates the foreground PID of the respective terminal; it does not deliver POSIX signals to process groups.

## 6. TTY and I/O

PicoNIX supports the USB CDC console and a UART1 terminal. For example, `getty /dev/uart1 sh &` starts another Shell. Each terminal tracks its owning Shell and foreground PID; `Ctrl+C` acts on that terminal's foreground task.

Use `ioctl()` to set **terminal input modes** (canonical/noncanonical and echo) and `fcntl()` to set **per-FD nonblocking flags**. These settings are independent. See [Terminal settings and interactive input](TTY.md) for examples. This is not a full POSIX `termios` implementation: `VMIN` and `VTIME` are not provided.

## 7. Virtual files and devices

| Path | Purpose |
|---|---|
| `/bin` | Read-only list of Flash-embedded registered programs |
| `/tmp` | Small RAM files, lost on reboot |
| `/dev/console`, `/dev/uart1` | Terminal I/O |
| `/dev/gpio/N`, `/dev/adc/*`, `/dev/pwm/*` | GPIO, ADC and PWM |
| `/dev/mem` | Diagnostic memory reads |
| `/dev/trace` | Binary tracing records |
| `/proc/tasks`, `/proc/uptime`, `/proc/interrupts`, `/proc/memory` | Kernel status |

See [Devices and wiring](DEVICES.md) for device paths and pin assignments. Each task has its own descriptors, and the same `read()` and `write()` APIs operate on files and devices.

## 8. Tracing and debugging

The kernel uses a ring buffer of 1,024 records of 16 bytes each (16 KiB total). It records task switches and SVC entry/exit events. The `trace` command turns the records into readable text.

```text
trace on
hello
trace off
trace show
```

`/dev/trace` provides the binary records. See [Tracing](TRACE.md) for event formats and interpretation.

## 9. Where to look in the source

| File | Responsibility |
|---|---|
| `kernel/main.c` | Startup and application registration |
| `kernel/kernel.c` | Tasks, SVC, descriptors, `/proc`, `/tmp`, TTY |
| `kernel/context.S` | Exceptions and context switching |
| `kernel/gpio.c`, `adc.c`, `pwm.c`, `fb.c` | Device drivers |
| `kernel/pins.h` | Exposed pins and initial assignments |
| `userland/shell.c` | Command parsing and redirection |
| `userland/syscalls.c`, `crt0.c` | User-side SVC wrappers and startup |
| `userland/slot.ld`, `CMakeLists.txt` | RAM layout, images and PIC validation |

**Next:** [C application development](PROGRAMMING.md) for adding a command, [Devices and wiring](DEVICES.md) for hardware, and [Terminal settings and interactive input](TTY.md) for TTY details.
