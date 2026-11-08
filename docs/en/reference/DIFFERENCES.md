# How PicoNIX differs from Linux

*Technical reference*

PicoNIX is a **small educational UNIX-like operating system**, not a Linux distribution or a POSIX-compatible replacement. It retains useful UNIX ideas—processes, file descriptors, device files, pipes, terminals, and system calls—while deliberately omitting or simplifying other mechanisms. The differences below describe the current source tree, not promises of Linux compatibility.

## 1. Processes, signals, and job control

| Topic | Linux | PicoNIX |
|---|---|---|
| Program launch | Typically `fork()` followed by `execve()`, or other process-creation APIs | `spawn()` creates a task directly from a registered program; a separate mechanism launches supported RAM images |
| Address space | Each process normally has its own virtual address space | Six fixed physical SRAM task slots; no virtual memory or process address-space protection |
| Process groups and sessions | Process groups, sessions, and job-control APIs | **No POSIX process groups or sessions** |
| Signals | General signal delivery and handlers | Simplified `kill(pid)` termination and Ctrl+C foreground interruption; **no general POSIX signal facility** |
| Foreground pipelines | Job control normally operates on process groups | A small kernel list tracks up to three foreground task PIDs per TTY for Ctrl+C; **this is not a POSIX process group** |
| Background jobs | POSIX shell job-control facilities may be available | Basic `&`; no full `jobs`, `fg`, `bg`, or process-group job control |
| Task capacity | Depends on resources and system limits | **Six user tasks maximum**, shared by shells, services, and applications; there is also an internal idle task |

For example, `echo hello | cat` connects processes through a pipe. Ctrl+C can stop the foreground pipeline, but there is no `setpgid()`-based job control behind it. A task is started in a free slot rather than by duplicating the current task with `fork()`.

## 2. Users, permissions, and memory protection

PicoNIX has **no user accounts, UID/GID, root account, login authentication, or filesystem permission checks**. In that sense, all programs have root-like access, but **there is no actual root user**. Do not confuse a Linux root account (an OS permission concept) with CPU execution privilege (a hardware protection concept). On RP2040, PicoNIX also has no hardware-isolated user memory. These omissions make PicoNIX suitable for trusted experiments, **not for securely isolating untrusted users or programs**.

The kernel executes code from flash by XIP; built-in user PIC images are stored in flash and copied to SRAM slots for execution. Each 24 KiB slot reserves 4 KiB for its stack. This differs from Linux's ELF loading, virtual memory, and protected processes.

## 3. Files, directories, and I/O

PicoNIX has no disk filesystem, persistent writable flash filesystem, general mounting, symbolic links, or file ownership. Its VFS exposes:

| Path | What it represents |
|---|---|
| `/tmp` and `/etc` | Ordinary **volatile RAM files**, using one shared file pool; writes disappear on reboot |
| `/dev` | Kernel device interfaces such as TTY, GPIO, I²C, and (on Pico W) network capture/ICMP |
| `/proc` | Virtual read-only system and task information |
| `/bin` | Registered built-in program names, **not disk-resident executable files** |

`/etc` has preloaded configuration/HTML files and is still RAM-backed; those are not permanent disk files. Initial RAM files are installed again on reboot. Directories are provided by the VFS; arbitrary nested directory creation is not supported. Relative paths and `.`/`..` are handled, but full Linux filesystem semantics are not.

The familiar `open()`/`read()`/`write()`/`close()` model is present, along with pipes and a compact `FILE`-based C library. The 64-byte C stream buffers are a limited implementation, not glibc. Not every Linux/POSIX syscall or stdio feature is implemented.

## 4. Terminals and networking

| Topic | PicoNIX behavior |
|---|---|
| USB CDC | `/dev/console`; available on Pico and Pico W |
| UART1 | `/dev/ttyS1`, GP20 (TX) and GP21 (RX); available on Pico and Pico W |
| Wi-Fi Telnet | `/dev/ttyN0` on Pico W builds; kernel Telnet service on TCP port 23 |
| TTY modes | Simplified canonical/noncanonical input, echo, newline conversion, and nonblocking I/O; **not full POSIX termios** |
| TCP/UDP | A small IPv4 socket API via lwIP on Pico W; not complete BSD/POSIX sockets |
| ICMP and capture | Pico W `ping`, `tcpdump`, `/dev/net/icmp`, `/dev/net/packet`; capture contains IPv4 summaries and at most 16 payload bytes, **not full Ethernet frames or libpcap** |

Telnet and the UDP remote-device service do **not** provide authentication or encryption. Connect only on a trusted experimental network. The ordinary Pico does not contain Wi-Fi networking hardware.

## 5. Implementation limits

The following are **fixed implementation capacities**, not Linux-like resource-limit settings. Some resources are shared system-wide.

| Resource | Current PicoNIX limit |
|---|---|
| Target | RP2040 (Raspberry Pi Pico / Pico W) |
| User task slots | **6**, plus a separate internal idle task |
| RAM per user slot | **24 KiB**, including **4 KiB reserved for stack** |
| Kernel SRAM region | **56 KiB allocated address region**; not a measurement of actual kernel code size |
| User program build | PIC; `-Oz` optimization |
| Open file descriptors | **16 per task**, numbered 0–15 (includes stdin/out/err, pipes, sockets) |
| Pipe objects | **2** simultaneously, **256 bytes** per pipe |
| RAM files | **11 total** shared by `/tmp` and `/etc`, **2,048 bytes** each |
| RAM filename | At most **27 ASCII bytes** for a direct child name in `/tmp` or `/etc` |
| Shared memory | **2 KiB** at `/dev/shm` |
| Trace area | **16 KiB** |
| Network reservation | **16 KiB** (including lwIP heap and idle/network stack layout on Pico W) |
| TCP user socket slots | **4** on Pico W |
| Packet capture | **16** fixed records on Pico W |
| C stdio | **64-byte buffer per stream**; up to **4 additional `fopen()` streams** |
| TTY endpoints | **2** on Pico; **3** on Pico W, including Telnet |

These are capacities, not guarantees that every combination of processes, descriptors, sockets, and services will run simultaneously. See [memory layout](MEMORY-LAYOUT.md), [filesystem](FILESYSTEM.md), [TTY guide](commands/TTY.md), and [network devices](network/NETWORK.md) for details.

## 6. Source of truth

Limits and behavior were checked against `include/piconix.h`, `include/device_names.h`, `kernel/kernel.c`, `kernel/net_socket.h`, `userland/apps/shell.c`, and the existing [specification](SPECIFICATION.md). When these sources change, update both language versions and the corresponding reference pages. Hardware and full firmware build tests are separate from this documentation audit.

---

**Related links:** [Reference index](README.md) | [Learning roadmap](../ROADMAP.md) | [日本語](../../ja/reference/DIFFERENCES.md) | [Documentation guide](../README.md)
