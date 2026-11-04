# Kernel Development Guide

*Development guide*

To study the kernel before modifying it, see the [kernel reading guide](KERNEL-READING-GUIDE.md).

## Before you start

**Prerequisites:** Read [Architecture](../../reference/ARCHITECTURE.md), [Memory layout](../../reference/MEMORY-LAYOUT.md), and [Tracing](../../guides/TRACE.md). Begin by following task creation and termination in the existing source code.


[Documentation guide](../../../DOCS.md) | [Internals](../../reference/INTERNAL.md) | [System calls](../../reference/SYSCALLS.md) | [Devices](../../reference/devices/DEVICES.md) | [日本語](../../../ja/development/kernel/KERNEL-DEVELOPMENT.md)

This guide is for contributors **extending the PicoNIX kernel**. For adding a user-space C command, see [PROGRAMMING.md](../apps/c/PROGRAMMING.md). The procedures describe the source structure in the supplied revision, not a ready-to-apply patch. Preserve the existing ABI and hardware resource reservations.

## 1. Source map

| File | Responsibility |
|---|---|
| `kernel/kernel.c` | Tasks, SVC dispatch, FDs, VFS, device I/O, TTY |
| `kernel/main.c` | Registering and starting embedded user programs |
| `kernel/context.S` | Exceptions and context switching |
| `userland/lib/syscalls.c` | C-side SVC wrappers and BSD-like adapters |
| `userland/lib/unistd.h` | Public API declarations and constants |
| `include/piconix.h` | Shared types and resource limits |
| `kernel/gpio.c`, `kernel/adc.c`, `kernel/pwm.c`, `kernel/fb.c` | Device-specific backends |
| `kernel/pins.h`, `kernel/devices.h` | Pin roles and device definitions |
| `tools/run_tests.py`, `tools/test_scripts/` | Regression testing |

Read [ARCHITECTURE.md](../../reference/ARCHITECTURE.md) and [INTERNAL.md](../../reference/INTERNAL.md) for XIP, RAM slots and the SVC boundary. PicoNIX is not Linux; user pointers must be checked explicitly within its shared-address-space design.

## 2. Extension guides

- [Add a system call](../syscalls/SYSCALL-DEVELOPMENT.md) — ABI, dispatch, wrapper, and tests
- [Add a device driver](../drivers/DRIVER-DEVELOPMENT.md) — VFS registration, I/O, ioctl, and tests
- [Kernel networking (Pico W)](../network/NETWORK-DEVELOPMENT.md) — Wi-Fi, lwIP, sockets, and built-in services

The following worked exercise compares both extension mechanisms.

## 3. Complete exercise: one syscall and an independent virtual device

Expose the existing boot-time counter through **two different interfaces**: `getuptime()` (numeric SVC return) and `/dev/uptime` (read-only text via VFS). The existing `/proc/ticks` and `/proc/uptime` remain unchanged.

| Goal | Full practical guide | Final check |
|---|---|---|
| Add `getuptime()` SVC | [System call development](../syscalls/SYSCALL-DEVELOPMENT.md) | `labuptime`, `trace run labuptime` |
| Add `/dev/uptime` and a C driver module | [Device node and driver](../drivers/DRIVER-DEVELOPMENT.md) | `ls /dev`, `cat /dev/uptime`, EOF |
| Register and flash an app | [C programming](../apps/c/PROGRAMMING.md) and [Setup](../../setup/SETUP.md) | `ls /bin`, device execution |

The two dedicated guides contain the full changes, commands, expected observations, failure cases, and the optional `docs/examples/uptime_lab.patch`. Apply the example patch **only on a separate exercise branch**. Avoid repeating long source fragments here; the dedicated labs are authoritative to prevent drift.

**Exercise:** Draw both paths from the 1 ms kernel counter: through SVC returning an integer, and through VFS/FD returning formatted text. No physical hardware is connected in this virtual-driver exercise.


## 4. Related documents

- [SYSCALLS.md](../../reference/SYSCALLS.md): ABI, arguments and return values
- [LIBC.md](../apps/c/LIBC.md): C library and wrappers
- [DEVICES.md](../../reference/devices/DEVICES.md): device nodes and APIs; [PINS.md](../../reference/hardware/PINS.md): pin assignments; [WIRING.md](../../reference/hardware/WIRING.md): physical wiring
- [INTERNAL.md](../../reference/INTERNAL.md): Tasks, VFS and internals
- [PROGRAMMING.md](../apps/c/PROGRAMMING.md): User-space C development
- [TRACE.md](../../guides/TRACE.md): Inspecting SVCs and task switches

---

**Related links:** [Development guide index](../README.md) | [Part C textbook](../../learn/C/README.md)
