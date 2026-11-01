# PicoNIX documentation


**[Current specification](SPECIFICATION.md)** — developer reference for memory layout, resource limits, ABI, and networking.

[Project overview](../../README.md) | [日本語](../ja/DOCS.md)

PicoNIX is a compact UNIX-like OS for RP2040. For a first visit, follow **setup → shell basics → OS concepts → programming**. The guides below describe the current interface; development-stage notes are historical.

## 1. Get started

1. [Setup](SETUP.md) — build, flash, and connect
2. [Overview and first commands](README.md) — `help`, `ls`, `cat`, `ps`, and other shell commands
3. [Devices and wiring](DEVICES.md) — GPIO, PWM, ADC, LED matrix
4. [Filesystem and node reference](FILESYSTEM.md) — directories, `/dev`, `/proc`, and RAM file limits

## 2. Learn UNIX concepts

| Topic | Guide |
|---|---|
| Pipes, redirection, descriptors, EOF | [Pipes and standard I/O](PIPE.md) |
| Canonical/raw input, `stty`, `fcntl()`, `O_NONBLOCK` | [TTY](TTY.md) |
| ANSI display control and interruption | [Terminal](TERMINAL.md) |
| USB console, UART1, and `getty` | [UART](UART.md) |
| System-call tracing with `trace` | [Trace](TRACE.md) |
| Read-only memory inspection with `mem` | [Memory viewer](MEM.md) |

## 3. Use networking (Pico W)

1. [Wi-Fi modes](WIFI-MODES.md) — AP and STA
2. [Networking](NETWORK.md) — connecting and using services
3. [Socket programming](NETWORK-PROGRAMMING.md) — TCP/UDP user applications
4. [nc](NC.md), [httpd](HTTPD.md), [UDP device control](UDP-DEVICES.md), [Remote GPIO](REMOTE_GPIO.md) — practical examples

**[Architecture](ARCHITECTURE.md)** — the user/kernel boundary and a source-file map.

## 4. Write programs

| Topic | Guide |
|---|---|
| Edit text files with `edit` | [Text editor](EDIT.md) |
| Build a C application | [Programming](PROGRAMMING.md) |
| Available C headers, functions, parameters, return values, and limitations | [C library and APIs](LIBC.md) |
| SVC ABI, call numbers, arguments, and return values | [System calls](SYSCALLS.md) |
| `as`, `svc #0`, and Thumb assembly; see also the [SVC ABI](SYSCALLS.md) | [Assembler](ASM.md) |
| Use the Python-like interpreter | [Python](PYTHON.md) |
| Understand tasks, memory, and the kernel | [Internals](INTERNAL.md) |

For C applications, a useful reading sequence is **Programming → C library and APIs → System calls**. For assembly, start with **Assembler → System calls**.

- [Kernel development](KERNEL-DEVELOPMENT.md): Adding system calls and device nodes

## Development history

Earlier designs, implementation milestones, and debugging notes are collected in the [development history](../history/README.md). These are not current specifications.
