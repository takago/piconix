# Kernel Development Guide

*Development guide*

To study the kernel before modifying it, see the [kernel reading guide](KERNEL-READING-GUIDE.md).

## Before you start

**Prerequisites:** Read [Architecture](../../reference/ARCHITECTURE.md), [Memory layout](../../reference/MEMORY-LAYOUT.md), and [Tracing](../../reference/commands/TRACE.md). Begin by following task creation and termination in the existing source code.


[Documentation guide](../../README.md) | [Internals](../../reference/INTERNAL.md) | [System calls](../../reference/SYSCALLS.md) | [Devices](../../reference/devices/README.md) | [日本語](../../../ja/development/kernel/KERNEL-DEVELOPMENT.md)

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


**Exercise:** Draw both paths from the 1 ms kernel counter: through SVC returning an integer, and through VFS/FD returning formatted text. No physical hardware is connected in this virtual-driver exercise.



## 4. Change and verification workflow

Before editing, trace the relevant **entry point (SVC/VFS/driver) → internal state → user-visible result**, and check declarations shared with `include/` and `userland/lib/`. Avoid unnecessary cross-module dependencies or exposing `static` helpers merely for convenience.

1. Inspect `git status`, create a working branch if needed, and record baseline build and command behavior.
2. Make the smallest change possible. Recheck syscall numbers, event ABI, device IDs, SRAM layout and PIC constraints; update the corresponding reference if any of these change.
3. Run `python3 tools/run_tests.py` for host-side tests. Record passes, failures and tests not run separately; some tests require host tools or configurations that may not be available.
4. Build the target board using [Setup](../../setup/SETUP.md). If a shared change affects both Pico and Pico W, build both. Retain the build output including PIC and memory-layout checks.
5. Verify the changed path on hardware. For example, use `trace run hello` to observe syscall entry/exit, or exercise the affected `/dev` node for a driver. Record the board and build configuration with the result.

**Verification boundaries:** Passing host tests does not prove hardware behavior; passing a build does not, by itself, establish the status of every PIC, SRAM-layout or trace check.

## 5. Related documents

- [SYSCALLS.md](../../reference/SYSCALLS.md): ABI, arguments and return values
- [LIBC.md](../apps/c/LIBC.md): C library and wrappers
- [README.md](../../reference/devices/README.md): device nodes and APIs; [PINS.md](../../reference/hardware/PINS.md): pin assignments; [WIRING.md](../../reference/hardware/WIRING.md): physical wiring
- [INTERNAL.md](../../reference/INTERNAL.md): Tasks, VFS and internals
- [PROGRAMMING.md](../apps/c/PROGRAMMING.md): User-space C development
- [TRACE.md](../../reference/commands/TRACE.md): Inspecting SVCs and task switches

---

**Related links:** [Development guide index](../README.md) | [Learning roadmap](../../ROADMAP.md)
