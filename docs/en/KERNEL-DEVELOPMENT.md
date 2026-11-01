# Kernel Development Guide

[Documentation guide](DOCS.md) | [Internals](INTERNAL.md) | [System calls](SYSCALLS.md) | [Devices](DEVICES.md) | [日本語](../ja/KERNEL-DEVELOPMENT.md)

This guide is for contributors **extending the PicoNIX kernel**. For adding a user-space C command, see [PROGRAMMING.md](PROGRAMMING.md). The procedures describe the source structure in the supplied revision, not a ready-to-apply patch. Preserve the existing ABI and hardware resource reservations.

## 1. Source map

| File | Responsibility |
|---|---|
| `kernel/kernel.c` | Tasks, SVC dispatch, FDs, VFS, device I/O, TTY |
| `kernel/main.c` | Registering and starting embedded user programs |
| `kernel/context.S` | Exceptions and context switching |
| `userland/syscalls.c` | C-side SVC wrappers and BSD-like adapters |
| `userland/unistd.h` | Public API declarations and constants |
| `include/piconix.h` | Shared types and resource limits |
| `kernel/gpio.c`, `kernel/adc.c`, `kernel/pwm.c`, `kernel/fb.c` | Device-specific backends |
| `kernel/pins.h`, `kernel/devices.h` | Pin roles and device definitions |
| `tools/run_tests.py`, `tools/test_scripts/` | Regression testing |

Read [ARCHITECTURE.md](ARCHITECTURE.md) and [INTERNAL.md](INTERNAL.md) for XIP, RAM slots and the SVC boundary. PicoNIX is not Linux; user pointers must be checked explicitly within its shared-address-space design.

## 2. Adding a system call

### 2.1 Allocate a number and define the ABI

`kernel/kernel.c` declares `SYS_GETPID` onward in an enum covering numbers 1–38. **Numbers 11 and 15 are reserved.** Never reorder or reuse existing numbers. Append a new explicitly numbered entry rather than shifting existing values. The user wrappers in `userland/syscalls.c` use numeric syscall IDs, so mismatches break the ABI.

The calling convention is `r0` = syscall number, `r1`–`r3` = arguments, `svc #0`, and return value in `r0`. Use a pointer to a validated structure if more argument fields are needed. Errors normally return negative `OS_E*` values rather than setting `errno`. See [SYSCALLS.md](SYSCALLS.md).

### 2.2 Implement the kernel handler

`os_svc_dispatch(uint32_t *f)` in `kernel/kernel.c` dispatches on `switch(num)` and sets `result`. Follow simple existing cases such as `SYS_FLASH_ID` or `SYS_GETPID`.

```c
/* Schematic placement only; do not paste without defining the symbols. */
case SYS_EXAMPLE:
    result = example_kernel_operation(a);
    break;
```

`a`, `b` and `c` correspond to `r1`–`r3`. Before dereferencing user pointers, validate **length, address range, termination and integer overflow** using existing patterns such as `in_task_range()` and path validation. Never dereference an arbitrary user-supplied address. Review interrupt protection and scheduling rules when modifying shared state or blocking a task.

### 2.3 Export the C wrapper

`call(n, a, b, c)` in `userland/syscalls.c` issues `svc #0`. Add a C wrapper and a matching prototype to the appropriate public header, such as `userland/unistd.h`. Keep parameter types, register order and error results consistent with the kernel. Update both language versions of [SYSCALLS.md](SYSCALLS.md) and [LIBC.md](LIBC.md).

### 2.4 Test the ABI

- Existing numbers and reserved entries remain unchanged; the new number is unique
- Zero, boundary, invalid-FD, invalid-pointer and oversized inputs return defined errors
- Failures do not corrupt task state, FDs or buffers
- Wrapper argument positions match the dispatcher
- SVC events appear in `trace` where relevant

## 3. Adding a device node

### 3.1 Understand the VFS path

In `kernel/kernel.c`, fixed `/dev` paths live in `dev_registry[]`; fixed special entries including `/proc` live in `vfs_registry[]`. `vfs_open()` resolves paths and installs a device ID in the task's FD table. `vfs_rw()` dispatches reads and writes, `device_ioctl()` handles control requests, and FD release handles cleanup.

**Adding a registry entry alone does not implement read or write.** GPIO, ADC and PWM also use dynamically resolved or conditionally exposed paths. Verify enumeration through `/dev` and, where applicable, `/proc/devices`.

### 3.2 Example workflow for a read-only device

1. Choose a free `DEV_*` ID; check for collisions with existing device IDs, internal pipe IDs, directory IDs and symbol IDs.
2. For a fixed path, add `"/dev/example"` and its ID to `dev_registry[]`. `/proc` entries use a different registry and enumeration path.
3. Enforce read-only open modes in `vfs_open()`; review existing `OS_O_RDONLY` and `OS_O_NONBLOCK` constraints.
4. Add a branch in `vfs_rw()` for the ID. Reject writes with `OS_EBADF`, return the number of bytes read on success, and negative `OS_E*` codes on failure. The dispatcher checks user buffer ranges, but backends must also validate their own sizes and state.
5. For sequential reads, define offset and EOF behavior using `tasks[current].fd_pos[fd]`; implement `lseek()` support only if required.
6. Extend `device_ioctl()` for any supported commands; leave unsupported commands as errors.
7. For hardware devices, check `kernel/pins.h` and driver ownership: do not conflict with USB, UART, Wi-Fi, framebuffer or PWM resources.
8. Update both language versions of `DEVICES.md`, `FILESYSTEM.md`, and `LIBC.md` where relevant.

This is an **application-side check**, not the device implementation:

```c
#include "unistd.h"
#include "fcntl.h"

int fd = open("/dev/example", O_RDONLY);
if (fd >= 0) {
    char buf[32];
    int n = read(fd, buf, sizeof buf);
    /* n > 0: data; n == 0: EOF; n < 0: error */
    close(fd);
}
```

Confirm actual public macro names and types against the headers when implementing the feature.

### 3.3 Device tests

| Area | Verification |
|---|---|
| Paths | Correct paths open; invalid paths fail |
| Enumeration | Appears in `ls /dev` and `/proc/devices` where applicable |
| Access | Writes to a read-only FD fail |
| Buffers | Zero-length, boundary and invalid-pointer cases are safe |
| Lifetime | Repeated open/close and task exit preserve FD consistency |
| Contention | Ownership and concurrent access rules are defined |
| Hardware | Missing peripherals are handled safely on Pico and Pico W |

## 4. Worked exercise: adding `getuptime()` and `/dev/uptime`

This exercise exposes **the same time since boot through two interfaces**: a numeric system call and a text-based device node. No extra hardware is needed. The snippets are **source-change instructions, not an applied patch**; they have not been compiled or tested on hardware.

### 4.1 Existing facilities and design choices

The kernel already maintains `ticks` at 1-ms intervals and exports `/proc/ticks` as an integer and `/proc/uptime` as formatted seconds. Adding `/dev/uptime` is intentionally redundant: the goal is to compare how syscalls and VFS nodes are extended, not to introduce a necessary production feature.

| New interface | Behavior | Existing comparison |
|---|---|---|
| `unsigned getuptime(void)` | 32-bit milliseconds since boot | `/proc/ticks` |
| `/dev/uptime` | Text such as `123.456 s\n` | `/proc/uptime` |

The `uint32_t` counter wraps after approximately 49.7 days. The wrapper returns an `unsigned` value to preserve all 32 bits. For this exercise, use syscall **39** and a currently unallocated internal device ID **264**. Recheck these numbers against the current tree before implementing.

### 4.2 Add the system call

Append to the `SYS_*` enumeration in `kernel/kernel.c`, preserving the existing 1–38 numbering:

```c
SYS_PROGRAM_ID, SYS_GETUPTIME = 39
```

In `os_svc_dispatch()`'s `switch(num)`, add:

```c
case SYS_GETUPTIME:
    result = (int)ticks; /* preserve 32-bit value in r0 */
    break;
```

Add a wrapper to `userland/syscalls.c`, using the existing `call()` helper:

```c
unsigned getuptime(void) {
    return (unsigned)call(39, 0, 0, 0);
}
```

Declare it in `userland/unistd.h`:

```c
unsigned getuptime(void); /* milliseconds, wraps at 32 bits */
```

This call has no arguments. Every 32-bit result is a valid timer value, so negative-result error checking cannot be used; the design does not offer separate error reporting for this call.

### 4.3 Add a read-only `/dev/uptime`

Reuse the existing `DEV_PROC_UPTIME` formatter and per-FD read cursor in `device_rw_proc()` rather than duplicating them. Make these edits in `kernel/kernel.c`:

1. Add `DEV_UPTIME = 264` to the device-ID enumeration, checking it does not overlap existing IDs or ranges.
2. Add this entry to `dev_registry[]` to make the node resolvable and visible through `/dev` and `/proc/devices`:

```c
{"/dev/uptime", DEV_UPTIME},
```

3. Include `dev==DEV_UPTIME` in the read-only access check in `vfs_open()`. Opening it with `O_WRONLY` or `O_RDWR` must fail.
4. Add this branch to `vfs_rw()` **before the GPIO fallback**:

```c
if (dev == DEV_UPTIME)
    return device_rw_proc(fd, DEV_PROC_UPTIME, p, n, writing);
```

The existing proc read helper advances `fd_pos[fd]`, returns `0` at EOF, and rejects writes with `OS_EBADF`. Merely adding a path to `dev_registry[]` does not implement its I/O behavior.

### 4.4 Compare the two user interfaces

To register a new user-space C command, follow [PROGRAMMING.md](PROGRAMMING.md). First try:

```text
cat /proc/ticks
cat /proc/uptime
cat /dev/uptime
```

A minimal C example (see [LIBC.md](LIBC.md) for the supported interfaces):

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

- Existing syscall numbers 1–38 and reserved slots 11 and 15 remain unchanged; 39 is unique.
- `getuptime()` roughly matches `/proc/ticks` and advances over time.
- `cat /dev/uptime` has the same format as `/proc/uptime`.
- `ls /dev` and `cat /proc/devices` list the new node.
- Write-mode open is rejected; check zero-length and short reads, EOF, and repeated open/close.
- Run `python3 tools/run_tests.py`, then `python3 tools/run_tests.py --all` with the ARM toolchain, and test on Pico/Pico W hardware.
- Update both languages of `SYSCALLS.md`, `LIBC.md`, and `DEVICES.md`; check existing tests for fixed syscall and device-ID assumptions.

The goal is to learn **two ways of exposing existing kernel state**, not to add a new timekeeping subsystem.

## 5. Changes, verification and documentation

```bash
python3 tools/run_tests.py
python3 tools/run_tests.py --all
```

The `--all` tests also need an ARM toolchain. Use [SETUP.md](SETUP.md) for SDK setup and complete firmware builds. **Passing static tests does not prove hardware behavior.** Check actual boards separately for device I/O, concurrency and longer operation.

Recommended checklist: kernel implementation → user wrappers and headers → tests → English and Japanese references → README/DOCS links if new documents are added.

## 6. Related documents

- [SYSCALLS.md](SYSCALLS.md): ABI, arguments and return values
- [LIBC.md](LIBC.md): C library and wrappers
- [DEVICES.md](DEVICES.md): Device nodes and pin assignments
- [INTERNAL.md](INTERNAL.md): Tasks, VFS and internals
- [PROGRAMMING.md](PROGRAMMING.md): User-space C development
- [TRACE.md](TRACE.md): Inspecting SVCs and task switches
