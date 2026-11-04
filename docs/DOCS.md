# PicoNIX documentation guide

[Project overview](../README.md) | [日本語](DOCS.ja.md) | [Development diary](DEVELOPMENT_DIARY.md)

Use **guided courses** to learn in a coherent order, and **topic/reference guides** to look up details. The existing technical guides have not been moved or removed.

## Learn step by step

**Start with the [beginner–intermediate–advanced learning path](en/literacy/LEARNING-PATH.md).**

| Level | Course | Focus | Ready to progress when you can… |
|---|---|---|---|
| **Beginner** | [Use PicoNIX](en/literacy/BEGINNER.md) | Shell, files, pipes, processes | Run commands and explain their I/O |
| **Intermediate** | [Investigate PicoNIX](en/literacy/INTERMEDIATE.md) | FDs, terminals, memory, tracing, devices, network | Observe and explain OS behavior |
| **Advanced** | [Read and extend PicoNIX](en/literacy/ADVANCED.md) | C, SVC, VFS, kernel reading and extensions | Trace code paths and explain component responsibilities |

Begin with [Setup](en/setup/SETUP.md). Most labs work on a standard Pico. Networking (Pico W) and external hardware experiments are optional. No code modification is required for the first two levels.

## Reference and topic guides

The following documents are for looking up usage, implementation details, and current specifications during a course.

### Setup and overview

- [Setup](en/setup/SETUP.md): development environment, build and flashing
- [PicoNIX overview and commands](en/literacy/README.md): features and initial operations
- [Architecture](en/reference/ARCHITECTURE.md): components and execution model
- [Differences from Linux](en/reference/DIFFERENCES.md): omissions and limits
- [Specification](en/reference/SPECIFICATION.md): current behavior
- [Hardware](en/reference/hardware/HARDWARE.md): wiring and equipment

### Shell, files, terminals

- [UNIX basics](en/literacy/BASICS.md), [help](en/literacy/HELP.md)
- [Standard I/O and IPC](en/literacy/IO-IPC.md), [Pipes](en/literacy/PIPE.md)
- [Filesystem](en/reference/FILESYSTEM.md), [Initial RAM files and /etc/rc](en/reference/INITIAL-FILES.md)
- [Text editing](en/literacy/EDIT.md), [vi](en/literacy/VI.md), [fd file manager](en/literacy/FD.md), [dd and binary transfer](en/literacy/DD.md)
- [Terminal lab](en/literacy/TERMINALS-LAB.md), [Terminal usage](en/literacy/TERMINAL.md), [TTY settings](en/literacy/TTY.md), [Pong](en/literacy/PONG.md)

### Tasks, memory and observation

- [Processes](en/literacy/PROCESSES.md), [Memory and execution](en/literacy/MEMORY-EXECUTION.md)
- [System monitoring](en/literacy/MONITORING.md), [Trace](en/literacy/TRACE.md), [Memory viewer](en/literacy/MEM.md)
- [Internals](en/reference/INTERNAL.md), [Memory layout](en/reference/MEMORY-LAYOUT.md), [Syscalls](en/reference/SYSCALLS.md)
- [Kernel symbols](en/reference/KERNEL-SYMBOLS.md), [mem symbols](en/reference/MEM-SYMBOLS.md)

### Device I/O and hardware

- [Device I/O lab](en/literacy/DEVICE-IO.md), [Device reference](en/reference/devices/DEVICES.md)
- [I²C1](en/reference/devices/I2C.md), [UART](en/reference/devices/UART.md)
- [Remote device control](en/reference/devices/REMOTE-DEVICE.md), [Remote GPIO](en/reference/devices/REMOTE_GPIO.md), [UDP device control](en/reference/devices/UDP-DEVICES.md)

### Networking (Pico W)

- [Networking lab](en/literacy/NETWORKING.md), [Network access and reference](en/reference/devices/NETWORK.md)
- [Wi-Fi modes](en/reference/devices/WIFI-MODES.md), [Network command](en/reference/devices/NET.md)
- [nc](en/reference/devices/NC.md), [HTTP server](en/reference/devices/HTTPD.md)

## Programming and kernel development

These are primarily **advanced implementation guides**. Begin with the [advanced course](en/literacy/ADVANCED.md) for a reading order.

- **User applications:** [C programming](en/development/apps/c/PROGRAMMING.md), [C library and APIs](en/development/apps/c/LIBC.md), [Network programming](en/development/apps/c/NETWORK-PROGRAMMING.md)
- **On-device programming:** [Python-like interpreter](en/development/apps/python/PYTHON.md), [Thumb assembler](en/development/apps/asm/ASM.md)
- **Read the kernel:** [Kernel reading guide](en/development/kernel/KERNEL-READING-GUIDE.md) → [Function index (Japanese)](FUNCTIONS.md) → [Internals](en/reference/INTERNAL.md)
- **Extend the kernel:** [Adding syscalls](en/development/syscalls/SYSCALL-DEVELOPMENT.md), [Adding drivers](en/development/drivers/DRIVER-DEVELOPMENT.md), [Kernel development](en/development/kernel/KERNEL-DEVELOPMENT.md)

## History and design decisions

The [Development diary](DEVELOPMENT_DIARY.md) describes design decisions. The [History archive](history/README.md) includes older experiments and decisions that may no longer match the current system. For current API and limits, prefer the reference documents.
