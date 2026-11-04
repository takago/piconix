# Intermediate course — investigate how the OS works

[Learning path](LEARNING-PATH.md) | [Beginner](BEGINNER.md) | [Advanced](ADVANCED.md) | [日本語](../../ja/literacy/INTERMEDIATE.md)

**Course route:** [Terminal lab](TERMINALS-LAB.md) → [Memory and execution](MEMORY-EXECUTION.md) → [System monitoring](MONITORING.md) → [Device I/O](DEVICE-IO.md) → [Advanced course](ADVANCED.md). Use Next at the end of each lesson. Supplementary topics and the Pico W networking lab are optional.

**Goal:** Connect observed command behavior to file descriptors (FDs), terminals, tasks, memory, and devices without modifying source code.

**Prerequisite:** [Beginner-level](BEGINNER.md) operations. Sections 1–5 work over USB on either board. Section 6 is optional and requires Pico W.

## 1. Processes and file descriptors

Revisit [Processes](PROCESSES.md), [Standard I/O](IO-IPC.md), and [Pipes](PIPE.md), then inspect open FDs:

```text
piconix# ps
piconix# lsof
piconix# echo hello | cat -n
```

**Think:** What do FDs 0, 1, and 2 point to? Why can each pipeline stage run as a separate task? Consult [Filesystem](../reference/FILESYSTEM.md) and [Internals](../reference/INTERNAL.md) as needed.

## 2. Terminal input versus screen output

Work through the [terminal lab](TERMINALS-LAB.md):

```text
piconix# tty
piconix# stty
```

Only try `stty raw` after reading [TTY settings](TTY.md), and restore `stty cooked` afterward. Terminal input settings are managed by the kernel; application-side ANSI sequences control screen drawing. See [Terminal usage](TERMINAL.md).

## 3. RAM, Flash, and executable images

Read [Memory and execution](MEMORY-EXECUTION.md), then run:

```text
piconix# ps
piconix# mem -s
piconix# mem -s kernel
piconix# mem -s sh
```

Use the [memory viewer guide](MEM.md) before entering full-screen `mem`. **Observe memory; do not modify addresses.** Use [Memory layout](../reference/MEMORY-LAYOUT.md) to distinguish Flash PIC images from running RAM task slots.

## 4. Observe system calls

Follow [Monitoring](MONITORING.md) and [Trace](TRACE.md):

```text
piconix# trace run hello
piconix# cat /proc/uptime
```

**Think:** Does `hello` write to USB directly, or does `write()` request a kernel service? PIDs and timing values vary between runs.

## 5. Inspect device files

The first part of the [device I/O lab](DEVICE-IO.md) requires no external circuit:

```text
piconix# ls /dev
piconix# ls /dev/gpio
piconix# cat /proc/fb
```

Compare `/tmp` and `/dev`: both have names but represent different objects. Refer to the [device reference](../reference/devices/DEVICES.md) to distinguish `read()`, `write()`, and `ioctl()`. Check [wiring](../reference/hardware/HARDWARE.md) before controlling external hardware.

## 6. Observe networking (Pico W, optional)

Follow the [networking lab](NETWORKING.md) to investigate Wi-Fi, Telnet, HTTP, and user-process sockets. First read the [network access guide](../reference/devices/NETWORK.md). Telnet and remote device control lack authentication and encryption: use a **trusted experimental network only**. This section is not required to enter the advanced course.

## Check your understanding

1. Relate pipes, ordinary files, and TTYs to the common FD interface.
2. Explain what `ps`, `mem`, and `trace` each reveal.
3. Distinguish Flash executable images and RAM task slots.
4. Explain why `/tmp`, `/dev`, and `/proc` support common I/O operations despite different backing objects.
5. Distinguish device I/O from networking and explain what is Pico W-specific.

Move to the [advanced course](ADVANCED.md) after reviewing [Architecture](../reference/ARCHITECTURE.md) and [Differences from Linux](../reference/DIFFERENCES.md).

---

## Learning navigation

| ← Previous | ↑ Contents | Next → |
| :--- | :---: | ---: |
| [Processes](PROCESSES.md) | [Learning path](LEARNING-PATH.md) | [Terminal lab](TERMINALS-LAB.md) |
