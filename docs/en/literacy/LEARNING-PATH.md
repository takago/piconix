# PicoNIX learning path — ten topics

[日本語](../../ja/literacy/LEARNING-PATH.md) | [Documentation](../../DOCS.md) | [Setup](../setup/SETUP.md)

This guide helps undergraduate engineering students progress **from hands-on commands to observation, programming, and OS extension**. No prior OS internals knowledge is required. Keep Linux build-PC commands separate from commands entered at the PicoNIX terminal (`piconix#`).

**Before starting:** Use a Raspberry Pi Pico or Pico W, a data-capable USB cable, and a Linux build PC. Complete [Setup](../setup/SETUP.md). Networking (topic 8) requires Pico W; optional external-device experiments in topic 3 require verified wiring. Files in `/tmp` are RAM-backed and disappear on reboot.

## Ten learning topics

| Order | Topic | Start here | Learning outcome |
|---|---|---|---|
| 1 | UNIX commands | [Basics](BASICS.md) | Explain the shell, paths, and commands |
| 2 | Standard I/O and IPC | [Standard I/O and pipes](IO-IPC.md) | Distinguish FDs, redirection, and pipes |
| 3 | Devices and I/O | [Device I/O](DEVICE-IO.md) | Explain `/dev` and basic device APIs |
| 4 | Processes and tasks | [Processes](PROCESSES.md) | Explain PIDs and foreground/background jobs |
| 5 | Memory and execution | [Memory and execution](MEMORY-EXECUTION.md) | Relate RAM, Flash, PIC, and task slots |
| 6 | Terminals | [Terminal lab](TERMINALS-LAB.md) | Distinguish terminal settings and screen drawing |
| 7 | System monitoring | [Monitoring lab](MONITORING.md) | Observe the OS using `ps`, `mem`, `trace`, etc. |
| 8 | Networking | [Networking lab](NETWORKING.md) | Observe IP, TCP, Telnet, and HTTP (Pico W) |
| 9 | Programming | [Language choices](#9-choose-a-programming-language) | Compare C, Tiny Python, and assembly |
| 10 | OS extension | [Extension guides](#10-extend-the-os) | Understand syscalls, drivers, and kernel changes |

You need not finish everything in one session. Topics 1–7 work on an ordinary Pico. Topic 8 is optional and requires Pico W. **Topics 9–10 are extension pathways**; you need not study all languages or all modifications.

## Your first fifteen minutes

```text
piconix# help
piconix# pwd
piconix# ls /bin
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# ps
```

Which operations work on files and which reveal the current OS state?

## 9. Choose a programming language

| Language | Start here | Where you work |
|---|---|---|
| Tiny Python | [Python](../development/apps/python/PYTHON.md) | Short scripts and interactive input on PicoNIX |
| C | [C applications](../development/apps/c/PROGRAMMING.md) | Build on Linux, run on PicoNIX |
| Thumb assembly | [Assembler](../development/apps/asm/ASM.md) | Try short examples with PicoNIX's `as` |

Tiny Python is not CPython. Adding a C program requires rebuilding firmware. Assembly runs without process memory protection; start with small examples.

## 10. Extend the OS

Follow [adding syscalls](../development/syscalls/SYSCALL-DEVELOPMENT.md), [adding drivers](../development/drivers/DRIVER-DEVELOPMENT.md), and [kernel development](../development/kernel/KERNEL-DEVELOPMENT.md). Consult the [syscall specification](../reference/SYSCALLS.md) and [memory layout](../reference/MEMORY-LAYOUT.md) before changing code.

## Record your observations

For each topic, record what you executed, what output you observed, what it means, and any differences from your expectations. PIDs and measured values can vary between runs.

## Quick glossary

| Term | Meaning |
|---|---|
| Shell | Environment that accepts commands and launches programs |
| Process/task | Running program managed in PicoNIX's fixed RAM slots |
| FD | Number representing an open file or terminal; 0/1/2 are stdin/stdout/stderr |
| VFS | Common paths and operations for files and devices |
| System call | Interface from a user program to a kernel service |
| PIC | Code independent of its placement address |
| XIP | Execute code directly from Flash without copying it all into RAM |

**References:** [Specification](../reference/SPECIFICATION.md), [Devices](../reference/devices/DEVICES.md), and [Hardware](../reference/hardware/HARDWARE.md).
