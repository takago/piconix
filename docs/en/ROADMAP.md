# PicoNIX Learning Roadmap

This roadmap connects prior UNIX/OS knowledge to **using PicoNIX → observing behavior → reading source → modifying the system**. It links to practical guides and reference documents without duplicating them as separate exercise documents.

Begin with [Setup](setup/SETUP.md).
## Audience and prerequisites

These materials assume prior knowledge of **UNIX/Linux command-line usage, core OS concepts (processes, system calls, file descriptors and virtual memory), and C pointers and structures**. Basic TCP/IP knowledge is assumed for the optional Pico W networking material; ARM/Thumb and exception-handling details are needed for low-level kernel reading. This is not a general introduction to these topics.

The central goal is to connect familiar concepts to **their concrete implementation on RP2040**: fixed user slots, a flash-resident kernel and RAM-loaded user images, PIC, SVC/PendSV, VFS and lwIP integration. Similar names do not imply Linux-equivalent protection or virtual-memory semantics; consult [Differences from Linux](reference/DIFFERENCES.md).

**Suggested path:** Observe behavior with a command reference → consult the PicoNIX-specific reference → follow the corresponding source functions in a development guide → modify and test on a separate branch if appropriate.
Follow the steps in order if you are new to PicoNIX. Network topics marked optional require Pico W.

## 1. Use the system

| Step | What to learn and try | Read |
|---|---|---|
| 1 | Install PicoNIX and connect a USB console | [Setup](setup/SETUP.md) · [help and basic commands](reference/commands/HELP.md) |
| 2 | Browse and edit files | [Filesystem](reference/FILESYSTEM.md) · [edit](reference/commands/EDIT.md) |
| 3 | Connect the output of commands | [Pipes and FDs](reference/commands/PIPE.md) |
| 4 | Work with processes and terminals | [TTY/stty](reference/commands/TTY.md) · [Terminal screens](reference/commands/TERMINAL.md) |

**Try it:** Inspect `help`, list programs with `ls /bin`, then run `echo hello | cat`. Explain how output reaches the next command.

## 2. Observe the OS

| Step | What to learn and try | Read |
|---|---|---|
| 5 | Understand files, descriptors and VFS | [Filesystem](reference/FILESYSTEM.md) · [Pipes](reference/commands/PIPE.md) |
| 6 | Inspect memory and processes | [mem guide](reference/commands/MEM.md) · [ROM/Flash/SRAM layout](reference/MEMORY-LAYOUT.md) |
| 7 | Observe system calls | [trace guide](reference/commands/TRACE.md) · [Syscall reference](reference/SYSCALLS.md) |
| 8 | Use hardware devices | [Devices](reference/devices/DEVICES.md) · [Pin assignments](reference/hardware/PINS.md) · [Wiring](reference/hardware/WIRING.md) |
| 9 (optional) | Inspect Pico W networking | [Network connection](reference/network/NETWORK.md) · [ip](reference/network/IP.md) · [ping](reference/network/PING.md) · [tcpdump](reference/network/TCPDUMP.md) |

**Try it:** Run `trace run hello` and use the [trace guide](reference/commands/TRACE.md) to understand `ENTER` and `EXIT`. Read the [mem guide](reference/commands/MEM.md) before operating the interactive memory viewer.

## 3. Develop and extend PicoNIX

| Step | What to learn and try | Read |
|---|---|---|
| 10 | Write a C user program | [C programming](development/apps/c/PROGRAMMING.md) · [Library API](development/apps/c/LIBC.md) |
| 11 | Trace startup and scheduling in the source | [Kernel Reading Guide](development/kernel/KERNEL-READING-GUIDE.md) · [Internals](reference/INTERNAL.md) |
| 12 | Add a system call | [System call development](development/syscalls/SYSCALL-DEVELOPMENT.md) |
| 13 | Add a device node and driver | [Driver development](development/drivers/DRIVER-DEVELOPMENT.md) |
| 14 (optional) | Write a TCP/UDP app and inspect networking | [Socket programming](development/apps/c/NETWORK-PROGRAMMING.md) · [Network development](development/network/NETWORK-DEVELOPMENT.md) |
| 15 | Test a kernel modification | [Kernel development](development/kernel/KERNEL-DEVELOPMENT.md) |

**Try it:** Build and run the examples in the guides. Make kernel changes on a separate branch and compare behavior before and after.

## Find material by task

For a particular goal use the [Task index](TASKS.md). See [Command reference](reference/commands/README.md), [Technical reference](reference/README.md), and [Development guides](development/README.md) for detailed instructions.

---

[Documentation index](README.md) | [日本語](../ja/ROADMAP.md)

