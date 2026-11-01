# PicoNIX Specification

This document summarizes the **current implementation and fixed resource limits** for developers. For usage instructions, see the [documentation index](DOCS.md). PicoNIX does not claim full POSIX compatibility. The source code of the relevant revision takes precedence if behavior differs from this document.

[Architecture](ARCHITECTURE.md) maps the responsibilities to source files.

## 1. Platform and architecture

- Targets: Raspberry Pi Pico (RP2040) and Pico W.
- Kernel: code executes from flash via XIP; data and management structures reside in SRAM.
- User programs: fixed physical RAM slots; no general-purpose virtual memory or process memory isolation is specified.
- Directory structure: `/bin`, `/dev`, `/proc`, `/tmp`, `/etc`.

## 2. Memory and tasks

| Item | Current implementation |
|---|---|
| User task slots | 6 |
| Size per slot | 24 KiB (24,576 bytes) |
| Kernel SRAM | `0x20000000`–`0x2000FFFF` (64 KiB) |
| Task slot area | `0x20010000`–`0x20033FFF` (144 KiB) |
| Reserved networking area | `0x20038000`–`0x2003FFFF` (32 KiB) |
| Image/zero-initialized region check | Slot size minus 4 KiB |

The final 4 KiB of a slot is reserved for the stack. The networking address reservation does not imply Wi-Fi functionality on a non-Wi-Fi Pico build. The kernel also has an idle task, separate from the six user slots.

Internal task states are `UNUSED`, `READY`, `SLEEPING`, `DEAD`, and `BLOCKED`. See `kernel/kernel.c` for scheduling and wait-state transitions.

## 3. File descriptors and pipes

- Each task has 16 descriptor numbers (0–15); standard input/output/error normally use 0/1/2.
- At most two pipes, each with a 256-byte ring buffer.
- `pipe()`, `dup()`, and `dup2()` are provided. Descriptor duplication, inheritance, and release are kernel-managed.
- Pipes carry byte streams. See [Pipes](PIPE.md) for blocking, EOF, and endpoint lifetime behavior.
- Shell pipeline syntax is limited; it is not a fully POSIX-compatible shell.

## 4. TTY

The USB console, UART1, and (when enabled) Wi-Fi terminal are distinct devices. Canonical/raw input, echo, and ONLCR translation are TTY settings. Per-descriptor `O_NONBLOCK` is controlled through `fcntl()` independently of the TTY input mode. The ioctl interface is PicoNIX-specific and is not full POSIX termios.

See `OS_TTY_*` in `include/piconix.h`, [TTY](TTY.md), and [Terminal](TERMINAL.md).

## 5. System calls and ABI

The internal system-call table uses numbers 1–38; numbers 11 and 15 are reserved. Thumb `svc #0` takes the call number in `r0`, arguments in `r1`–`r3`, and returns a result in `r0`. These are PicoNIX ABI numbers, not Linux/POSIX numbers.

See [System calls](SYSCALLS.md). Authoritative implementation: `kernel/kernel.c` and `userland/syscalls.c`.

## 6. Programs and assembler

C user programs use the PicoNIX-specific build and placement pipeline. The bundled `as` is a small Cortex-M0+ Thumb assembler, not a replacement for GNU assembler. The immediate `0` in `svc #0` is not the syscall number. See [Assembler](ASM.md) for supported syntax, directives, limits, and output.

## 7. Networking

Pico W builds use lwIP. AP or STA mode is selected at build time. STA mode requires an SSID and password and obtains IPv4 configuration through DHCP. Status is exposed through `/proc/net/wifi`.

| Feature | Implementation |
|---|---|
| Telnet (TCP 23) | Kernel-side remote terminal |
| HTTP (TCP 80) | User-space `httpd` |
| UDP device control (UDP 2000) | Kernel-side service |
| Socket API | User-space TCP/UDP functionality; not fully BSD-compatible |
| User TCP socket slots | 4 (`TCP_USER_SLOTS` in `kernel/kernel.c`) |
| Internal UDP transfer size constant | 512 bytes (`NET_SOCKET_UDP_MAX`) |

See [Wi-Fi modes](WIFI-MODES.md), [Networking](NETWORK.md), and [Socket programming](NETWORK-PROGRAMMING.md). Telnet and UDP device control do not provide encrypted authenticated access; use trusted networks.

## 8. Devices and pseudo-files

`/dev` exposes console, UART, GPIO, PWM, ADC, and related interfaces; `/proc` exposes task and system information. Actual nodes depend on the build configuration. See [Filesystem and node reference](FILESYSTEM.md), [Devices](DEVICES.md), and [Internals](INTERNAL.md).

## 9. Errors and compatibility

`include/piconix.h` defines negative error values such as `OS_EINVAL`, `OS_EBADF`, `OS_EAGAIN`, `OS_EINTR`, and `OS_ENOSYS`. Do not assume every call follows POSIX `errno` conventions. Consult the implementation and individual API guides.

## 10. Sources and maintenance

Primary sources: `include/piconix.h`, `kernel/kernel.c`, `kernel/net_socket.h`, `userland/syscalls.c`, `userland/shell.c`, and `CMakeLists.txt`. When implementation changes, update this specification, relevant guides, both languages, and executable examples together.
