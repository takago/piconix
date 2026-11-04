# Advanced course — read and extend the OS

[Learning path](LEARNING-PATH.md) | [Intermediate](INTERMEDIATE.md) | [日本語](../../ja/literacy/ADVANCED.md)

**Course route:** [C application development](../development/apps/c/PROGRAMMING.md) → [Kernel reading](../development/kernel/KERNEL-READING-GUIDE.md) → [System call development](../development/syscalls/SYSCALL-DEVELOPMENT.md) → [Driver development](../development/drivers/DRIVER-DEVELOPMENT.md) → [Kernel development](../development/kernel/KERNEL-DEVELOPMENT.md). Use Next at the end of each lesson. Supplementary topics and the Pico W networking lab are optional.

**Goal:** Trace how a small C program invokes a kernel service through a system call; explain the responsibilities of key kernel modules and a safe approach to extension. Modifying the OS in the final exercise is optional.

**Prerequisites:** [Intermediate](INTERMEDIATE.md) concepts, basic C functions/pointers/structures, and a Linux build environment. Study Cortex-M0+ exceptions and Thumb assembly as needed. Read [Architecture](../reference/ARCHITECTURE.md), [Memory layout](../reference/MEMORY-LAYOUT.md), and [Differences from Linux](../reference/DIFFERENCES.md) first.

## 1. User programs and the C library

Begin with [C application development](../development/apps/c/PROGRAMMING.md) and consult the [C library](../development/apps/c/LIBC.md). Open these files on the Linux host:

```text
userland/apps/hello.c
userland/lib/syscalls.c
include/piconix_syscall.h
```

**Investigate:** How does `hello` reach `write()`? PicoNIX embeds user PIC images in Flash and executes them in RAM slots. Adding a C app requires rebuilding the firmware.

**Optional:** Try [Python](../development/apps/python/PYTHON.md) for on-device scripting or the [Thumb assembler](../development/apps/asm/ASM.md) for instruction-level exploration; neither is mandatory.

## 2. Follow a system call

Run `trace run hello` on PicoNIX and compare the trace with [Syscalls](../reference/SYSCALLS.md). Read source in this order:

```text
userland/apps/hello.c
  → userland/lib/syscalls.c
  → include/piconix_syscall.h
  → kernel/context.S (SVC)
  → kernel/kernel.c (os_svc_dispatch())
```

**Investigate:** Where are syscall numbers and arguments specified? How is the result returned? Distinguish the system-call path from a context switch.

## 3. Read the kernel

Use the [kernel reading guide](../development/kernel/KERNEL-READING-GUIDE.md), with the [function index](../../FUNCTIONS.md) as a lookup tool:

1. Boot and task creation: `kernel/main.c`, `kernel/kernel.c`
2. Scheduling and context switching: `kernel/kernel.c`, `kernel/context.S`
3. System calls, VFS and FDs: `kernel/context.S`, `kernel/kernel.c`
4. TTY and pipes: `kernel/kernel.c`, `userland/apps/shell.c`
5. **Devices:** `kernel/gpio.c`, `fb.c`, `adc.c`, `i2c.c`, `pwm.c`
6. **Networking (Pico W, optional):** `kernel/net.c`, `kernel/net_socket.c`

**Investigate:** Draw the path of a single `write()` from user code to the terminal, labeling the functions. You do not need to modify the kernel yet.

## 4. Design a small extension (optional)

The [kernel development guide](../development/kernel/KERNEL-DEVELOPMENT.md) includes a `getuptime()` and `/dev/uptime` exercise. Read [Adding system calls](../development/syscalls/SYSCALL-DEVELOPMENT.md) and [Adding drivers](../development/drivers/DRIVER-DEVELOPMENT.md), then identify:

- Kernel changes versus shared headers and user-side wrappers
- Why a syscall or `/dev` interface is suitable
- Input validation, memory limits, and potential impact on existing commands
- How to build, test on hardware, and update specifications

If you make changes, follow [Setup](../setup/SETUP.md) and the existing tests. Source inspection alone cannot guarantee correct hardware behavior.

## 5. Study networking applications (Pico W, optional)

**After device I/O**, follow [Network programming](../development/apps/c/NETWORK-PROGRAMMING.md). Compare `userland/apps/httpd.c` and `kernel/net_socket.c`; identify where `socket()`, `bind()`, `listen()`, and `accept()` are handled. Keep Wi-Fi initialization distinct from user-space TCP/UDP sockets.

## Check your understanding

1. Explain the major stages from launching `hello` to exit.
2. Draw the relationship among `write()`, SVC, VFS, and TTY.
3. Explain how the build and test scope differs for app and kernel modifications.
4. Describe the separate responsibilities of device I/O and networking.

**References:** [Syscalls](../reference/SYSCALLS.md), [Internals](../reference/INTERNAL.md), [Function index](../../FUNCTIONS.md), and the [Development diary](../../DEVELOPMENT_DIARY.md).

---

## Learning navigation

| ← Previous | ↑ Contents | Next → |
| :--- | :---: | ---: |
| [Device I/O](DEVICE-IO.md) | [Learning path](LEARNING-PATH.md) | [C application development](../development/apps/c/PROGRAMMING.md) |
