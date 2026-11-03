# Kernel Development Guide

## Before you start

**Prerequisites:** Read [Architecture](../../reference/ARCHITECTURE.md), [Memory layout](../../reference/MEMORY-LAYOUT.md), and [Tracing](../../literacy/TRACE.md). Begin by following task creation and termination in the existing source code.


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

The following worked exercise compares both extension mechanisms.

## 4. Worked exercise: adding `getuptime()` and `/dev/uptime`

This exercise exposes **the same time since boot through two interfaces**: a numeric system call and a text-based device node. No extra hardware is needed. The snippets are **source-change instructions, not an applied patch**; they have not been compiled or tested on hardware.

### 4.1 Existing facilities and design choices

The kernel already maintains `ticks` at 1-ms intervals and exports `/proc/ticks` as an integer and `/proc/uptime` as formatted seconds. Adding `/dev/uptime` is intentionally redundant: the goal is to compare how syscalls and VFS nodes are extended, not to introduce a necessary production feature.

| New interface | Behavior | Existing comparison |
|---|---|---|
| `unsigned getuptime(void)` | 32-bit milliseconds since boot | `/proc/ticks` |
| `/dev/uptime` | Text such as `123.456 s\n` | `/proc/uptime` |

The `uint32_t` counter wraps after approximately 49.7 days. The wrapper returns an `unsigned` value to preserve all 32 bits. For this exercise, use syscall **40** and a currently unallocated internal device ID **265**. ID 264 is reserved for the symbol node of program ID 64, and pipe IDs begin at 268. Recheck these numbers against the current tree before implementing.

### 4.2 Add the system call

Append to `PICONIX_SYSCALL_TABLE(X)` in `include/piconix_syscall.h`, preserving the existing 1–39 numbering (`SYS_SHUTDOWN_TCP` is number 39):

```c
    X(SYS_SHUTDOWN_TCP, 39, "shutdown") \
    X(SYS_GETUPTIME,    40, "getuptime")
```

In `kernel/kernel.c`, add the following branch to the `switch(num)` in `os_svc_dispatch()`:

```c
case SYS_GETUPTIME:
    result = (int)ticks; /* preserve 32-bit value in r0 */
    break;
```

Add a wrapper to `userland/lib/syscalls.c`, using its existing `call()` helper and the shared syscall-number header:

```c
unsigned getuptime(void) {
    return (unsigned)call(SYS_GETUPTIME, 0, 0, 0);
}
```

Declare it in `userland/lib/unistd.h`:

```c
unsigned getuptime(void); /* milliseconds, wraps at 32 bits */
```

This call has no arguments. Every 32-bit result is a valid timer value, so negative-result error checking cannot be used; the design does not offer separate error reporting for this call.

### 4.3 Add a read-only `/dev/uptime`

Reuse the existing `DEV_PROC_UPTIME` formatter and per-FD read cursor in `device_rw_proc()` rather than duplicating them. Make the following edits in the shared device header and `kernel/kernel.c`:

1. Add `DEV_UPTIME = 265` to the fixed device-ID enum in `include/device_names.h`, checking it does not overlap existing IDs or ranges.
2. Add the path to `PICONIX_FIXED_DEVICES(X)` in the same header. `kernel/kernel.c` generates `dev_registry[]` from that macro, so this makes the node resolvable and visible in `/dev` and `/proc/devices`:

```c
    X("/dev/i2c/1", DEV_I2C1) \
    X("/dev/uptime", DEV_UPTIME)
```

3. Include `dev==DEV_UPTIME` in the read-only access check in `vfs_open()`. Opening it with `O_WRONLY` or `O_RDWR` must fail.
4. Add this branch to `vfs_rw()` **before the GPIO fallback**:

```c
if (dev == DEV_UPTIME)
    return device_rw_proc(fd, DEV_PROC_UPTIME, p, n, writing);
```

The existing proc read helper advances `fd_pos[fd]`, returns `0` at EOF, and rejects writes with `OS_EBADF`. Merely adding a path to `dev_registry[]` does not implement its I/O behavior.

### 4.4 Compare the two user interfaces

To register a new user-space C command, follow [PROGRAMMING.md](../apps/c/PROGRAMMING.md). First try:

```text
cat /proc/ticks
cat /proc/uptime
cat /dev/uptime
```

A minimal C example (see [LIBC.md](../apps/c/LIBC.md) for the supported interfaces):

```c
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

int main(void) {
    unsigned ms = getuptime();
    printf("uptime: %u ms\n", ms);
    int fd = open("/dev/uptime", O_RDONLY);
    if (fd < 0) return 1;
    char buf[32];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, (unsigned)n);
    close(fd);
    return n < 0 ? 1 : 0;
}
```

The two readings will not be exactly equal, since they occur at different times. The reused proc formatter snapshots time **for each read**, not once at open, so partial reads can mix observations made at different times. A stable snapshot across multiple reads would require additional per-open state.

### 4.5 Verification checklist

- Existing syscall numbers 1–39 and reserved slots 11 and 15 remain unchanged; 40 is unique.
- `getuptime()` roughly matches `/proc/ticks` and advances over time.
- `cat /dev/uptime` has the same format as `/proc/uptime`.
- `ls /dev` and `cat /proc/devices` list the new node.
- Write-mode open is rejected; check zero-length and short reads, EOF, and repeated open/close.
- Run `python3 tools/run_tests.py`, then `python3 tools/run_tests.py --all` with the ARM toolchain, and test on Pico/Pico W hardware.
- Update both languages of `SYSCALLS.md`, `LIBC.md`, and `DEVICES.md`; check existing tests for fixed syscall number 40 and device ID 265 assumptions.


For firmware builds and toolchain setup, see [SETUP.md](../../setup/SETUP.md). Passing static tests does not guarantee hardware behavior; new peripherals or interrupt handlers also require concurrency and extended runtime testing on actual boards.

The goal is to learn **two ways of exposing existing kernel state**, not to add a new timekeeping subsystem.

## 5. Related documents

- [SYSCALLS.md](../../reference/SYSCALLS.md): ABI, arguments and return values
- [LIBC.md](../apps/c/LIBC.md): C library and wrappers
- [DEVICES.md](../../reference/devices/DEVICES.md): Device nodes and pin assignments
- [INTERNAL.md](../../reference/INTERNAL.md): Tasks, VFS and internals
- [PROGRAMMING.md](../apps/c/PROGRAMMING.md): User-space C development
- [TRACE.md](../../literacy/TRACE.md): Inspecting SVCs and task switches
