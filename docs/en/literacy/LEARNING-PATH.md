# PicoNIX learning path — beginner, intermediate, advanced

[日本語](../../ja/literacy/LEARNING-PATH.md) | [Documentation index](../../DOCS.md) | [Setup](../setup/SETUP.md)

Learn PicoNIX in three steps: **use the system → observe how it works → read and extend the source**. This page is the curriculum entry point. Detailed usage and specifications remain in the existing topic guides rather than being duplicated here.

| Level | Course | Focus | Prerequisites | Hardware |
|---|---|---|---|---|
| Beginner | **[Use PicoNIX](BEGINNER.md)** | Shell, files, standard I/O, pipelines, processes | None | Pico or Pico W, USB |
| Intermediate | **[Investigate PicoNIX](INTERMEDIATE.md)** | FDs, terminals, RAM, tracing, devices, networking | Beginner-level commands | Pico or Pico W; Pico W for networking only |
| Advanced | **[Read and extend PicoNIX](ADVANCED.md)** | C applications, SVC, VFS, kernel reading, device and network development | Intermediate concepts, basic C | Pico or Pico W, Linux development PC |

Complete the exercises in each course in order. Move to the next level when you can explain the end-of-level questions in your own words. You can also follow a particular topic after checking its prerequisites. Kernel modifications are not required for the first two levels.

## Preparation and conventions

- Follow [Setup](../setup/SETUP.md) to build, flash and connect over USB. A regular Pico is sufficient except for the optional network lab.
- Commands prefixed `piconix#` run **on PicoNIX**. Commands such as `git`, `rg` and build scripts run **on the Linux host**.
- `/tmp` and `/etc` are RAM-backed; changes disappear after reboot. Do device experiments only after checking wiring and voltage levels.
- PicoNIX is a teaching OS, not a drop-in Linux replacement. See [Differences from Linux](../reference/DIFFERENCES.md).

## Learning materials by level

| Course section | Subject | Existing lessons and guides |
|---|---|---|
| Beginner 1–2 | Commands and files | [Basics](BASICS.md), [help](HELP.md), [edit](EDIT.md), [vi](VI.md), [fd](FD.md) |
| Beginner 3 | Redirection and pipes | [Standard I/O](IO-IPC.md), [Pipes](PIPE.md) |
| Beginner 4 | Processes | [Processes](PROCESSES.md) |
| Intermediate 1–2 | FDs and terminals | [Standard I/O](IO-IPC.md), [Terminal lab](TERMINALS-LAB.md), [TTY settings](TTY.md) |
| Intermediate 3–4 | Memory and observation | [Memory and execution](MEMORY-EXECUTION.md), [Monitoring](MONITORING.md), [Trace](TRACE.md) |
| Intermediate 5 | Device I/O | [Device lab](DEVICE-IO.md), [Device reference](../reference/devices/DEVICES.md) |
| Intermediate 6 (optional) | Networking (Pico W) | [Networking lab](NETWORKING.md) |
| Advanced 1–2 | Programming and system calls | [C applications](../development/apps/c/PROGRAMMING.md), [C library](../development/apps/c/LIBC.md), [Syscalls](../reference/SYSCALLS.md) |
| Advanced 3 | Kernel reading | [Kernel reading guide](../development/kernel/KERNEL-READING-GUIDE.md), [Function index](../../FUNCTIONS.md) |
| Advanced 4–5 | Extensions and networking | [Adding syscalls](../development/syscalls/SYSCALL-DEVELOPMENT.md), [Drivers](../development/drivers/DRIVER-DEVELOPMENT.md), [Network programming](../development/apps/c/NETWORK-PROGRAMMING.md) |

**Optional paths:** Try the [Python-like interpreter](../development/apps/python/PYTHON.md) or the [Thumb assembler](../development/apps/asm/ASM.md) after learning the basic commands. Neither is a prerequisite for kernel development.

## Record what you learn

For each exercise, write down **(1) commands, (2) observed output, (3) the OS concept it illustrates, and (4) open questions**. PIDs, addresses and timing values vary; understanding matters more than matching sample numbers.

## Reference material

The courses define *learning order*. For exact options, APIs and limits, use the [topic and reference index](../../DOCS.md#reference-and-topic-guides). Existing documents are preserved in their current locations.

## Quick glossary

| Term | Meaning |
|---|---|
| Shell | Accepts commands and launches programs |
| Process/task | Running program managed in fixed RAM slots |
| FD | Number for an open file or terminal; 0/1/2 are stdin/stdout/stderr |
| VFS | Common path and I/O interface for files and devices |
| System call | Interface from a user program to a kernel service |
| PIC | Code that does not depend on one placement address |
| XIP | Execute code directly from Flash without copying all of it into RAM |
