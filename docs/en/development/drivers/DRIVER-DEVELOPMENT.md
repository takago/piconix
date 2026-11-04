# Device Driver Development

*Development guide*

## Before you start

**Prerequisites:** Read the [Device reference](../../reference/devices/DEVICES.md) for `/dev`, then use `open()` and `read()` in a [C application](../apps/c/PROGRAMMING.md). Driver changes require rebuilding and testing on hardware.


[Kernel development](../kernel/KERNEL-DEVELOPMENT.md) | [Device reference](../../reference/devices/DEVICES.md) | [日本語](../../../ja/development/drivers/DRIVER-DEVELOPMENT.md)

This guide explains how to extend device nodes and their kernel-side handlers; it is not a list of commands for using existing devices.

## 1. Adding a device node

### 1.1 Understand the VFS path

In `kernel/kernel.c`, fixed `/dev` paths are declared by `PICONIX_FIXED_DEVICES(X)` in `include/device_names.h` and expanded into `dev_registry[]`; fixed special entries including `/proc` live in `vfs_registry[]`. `vfs_open()` resolves paths and installs a device ID in the task's FD table. `vfs_rw()` dispatches reads and writes, `device_ioctl()` handles control requests, and FD release handles cleanup.

**Adding a registry entry alone does not implement read or write.** GPIO, ADC and PWM also use dynamically resolved or conditionally exposed paths. Verify enumeration through `/dev` and, where applicable, `/proc/devices`.

### 1.2 Example workflow for a read-only device

1. Choose a free `DEV_*` ID; check for collisions with existing device IDs, internal pipe IDs, directory IDs and symbol IDs.
2. For a fixed path, add `"/dev/example"` and its ID to `PICONIX_FIXED_DEVICES(X)` in `include/device_names.h`. `/proc` entries use a different registry and enumeration path.
3. Enforce read-only open modes in `vfs_open()`; review existing `OS_O_RDONLY` and `OS_O_NONBLOCK` constraints.
4. Add a branch in `vfs_rw()` for the ID. Reject writes with `OS_EBADF`, return the number of bytes read on success, and negative `OS_E*` codes on failure. The dispatcher checks user buffer ranges, but backends must also validate their own sizes and state.
5. For sequential reads, define offset and EOF behavior using `tasks[current].fd_pos[fd]`; implement `lseek()` support only if required.
6. Extend `device_ioctl()` for any supported commands; leave unsupported commands as errors.
7. For hardware devices, check `kernel/pins.h` and driver ownership: do not conflict with USB, UART, Wi-Fi, framebuffer or PWM resources. See [pin assignments](../../reference/hardware/PINS.md) for the standard hardware configuration.
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

### 1.3 Device tests

| Area | Verification |
|---|---|
| Paths | Correct paths open; invalid paths fail |
| Enumeration | Appears in `ls /dev` and `/proc/devices` where applicable |
| Access | Writes to a read-only FD fail |
| Buffers | Zero-length, boundary and invalid-pointer cases are safe |
| Lifetime | Repeated open/close and task exit preserve FD consistency |
| Contention | Ownership and concurrent access rules are defined |
| Hardware | Missing peripherals are handled safely on Pico and Pico W |

---

**Related links:** [Development guide index](../README.md) | [Part C textbook](../../learn/C/README.md)
