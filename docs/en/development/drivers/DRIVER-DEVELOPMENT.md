# Device node and driver development: implement `/dev/uptime`

This exercise adds a **virtual read-only device** with its own C driver module. It requires no wires or extra hardware, and thus works on both Pico and Pico W. The purpose is to learn the difference between registering a path, opening a file descriptor, dispatching I/O and implementing a device backend. Pair it with the [getuptime syscall exercise](../syscalls/SYSCALL-DEVELOPMENT.md).

## 1. Data path and prerequisites

```text
cat /dev/uptime → vfs_open() → DEV_UPTIME → vfs_rw()
→ uptime_device_read() → "12.345 s\n" → user read()
```

`/proc/uptime` already exists. This deliberate duplication is for the lab. Before modifying code, read the [File system](../../reference/FILESYSTEM.md) and [Device reference](../../reference/devices/DEVICES.md).

## 2. Register the ID and path

In `include/device_names.h`, add an unused ID in the enum:

```c
    DEV_NET_ICMP = 145,
    DEV_UPTIME = 265,
```

In `PICONIX_FIXED_DEVICES(X)`, append the node; the previous line needs a trailing backslash:

```c
    X("/dev/i2c/1", DEV_I2C1) \
    X("/dev/uptime", DEV_UPTIME)
```

In the checked source, 200–263 are user symbol IDs and internal pipe IDs start at 268. **Verify 265 is still free in your version.** Registration puts the path into `/dev` enumeration but does not implement reading.

## 3. Write the independent driver module

Create `kernel/uptime_device.h`:

```c
/* Simple read-only virtual device used by the documentation exercise. */
#ifndef PICONIX_UPTIME_DEVICE_H
#define PICONIX_UPTIME_DEVICE_H
#include <stdint.h>
#include <stddef.h>
int uptime_device_read(void *dst, size_t capacity, uint32_t *position, uint32_t ticks_ms);
#endif
```

Create `kernel/uptime_device.c`:

```c
/* Read-only virtual driver.  No hardware pins are used. */
#include "uptime_device.h"
#include "../include/piconix.h"
#include <stdio.h>
#include <string.h>

int uptime_device_read(void *dst, size_t capacity, uint32_t *position, uint32_t ticks_ms) {
    char text[32];
    int len = snprintf(text, sizeof text, "%lu.%03lu s\n",
                       (unsigned long)(ticks_ms / 1000u),
                       (unsigned long)(ticks_ms % 1000u));
    if (len < 0 || (size_t)len >= sizeof text) return OS_EINVAL;
    if (*position >= (uint32_t)len || capacity == 0) return 0;
    size_t count = (size_t)len - *position;
    if (count > capacity) count = capacity;
    memcpy(dst, text + *position, count);
    *position += (uint32_t)count;
    return (int)count;
}
```

`position` is per-FD. Short reads advance it, and when the formatted string has been exhausted the driver returns 0 (EOF). A zero-size read returns 0. This simple example re-formats the changing counter on every read, so a multi-part read is **not an atomic snapshot**. A production design needing consistent timestamps must capture a value on open.

## 4. Add the module to the kernel build

In `CMakeLists.txt`, add `kernel/uptime_device.c` to the *kernel* `add_executable(piconix ...)` source list, **not** the user-app `foreach(APP IN ITEMS ...)` list. The following is a location hint, not a literal CMake line to paste:

```cmake
add_executable(piconix ... kernel/kernel.c kernel/uptime_device.c kernel/gpio.c ...)
```

## 5. Connect VFS to the driver

In `kernel/kernel.c`, include:

```c
#include "uptime_device.h"
```

In `vfs_open()`, include `dev==DEV_UPTIME` in the read-only checks, e.g. the end of the existing condition becomes:

```c
... || dev==DEV_TRACE || dev==DEV_UPTIME) &&
    (flags & 3)!=OS_O_RDONLY)) return OS_EINVAL;
```

In `vfs_rw()`, before the final unsupported-device fallback:

```c
if (dev == DEV_UPTIME) {
    if (writing) return OS_EBADF;
    return uptime_device_read(p, n, &tasks[current].fd_pos[fd], ticks);
}
```

The common VFS path validates the caller's user buffer; the new backend still handles its own count, position and EOF. No `ioctl` is needed in this example.

## 6. Build and check from the PicoNIX shell


```text
piconix# ls /dev
... uptime ...
piconix# cat /proc/devices
... uptime ...
piconix# cat /dev/uptime
12.345 s
piconix# labuptime
getuptime: 12399 ms
12.399 s
```

These are **illustrative outputs**. Confirm read-only opens, short reads, EOF, repeated open/close, unknown-path rejection and no changes to existing devices. The results are not expected to be numerically identical because time advances between calls.

After applying the optional patch, test the virtual driver's **short reads and EOF** on the Linux PC, without the Pico SDK:

```sh
cc -std=c11 -Wall -Wextra -Werror -Ikernel -Iinclude \
  docs/examples/test_uptime_device.c kernel/uptime_device.c -o /tmp/test_uptime_device
/tmp/test_uptime_device
```

A successful host-side unit test prints `PASS: uptime virtual device short reads and EOF`. This tests the driver logic only, **not** the full kernel VFS or hardware.

## 7. Extending the pattern to physical hardware

This lab shows a **virtual driver**, not a real GPIO electrical driver. For a physical device, trace existing `kernel/gpio.c` (`gpio_driver_rw`), `kernel/i2c.c`, `kernel/pins.h`, and the VFS dispatch in `kernel/kernel.c`. Before wiring or accessing a physical pin, consult [PINS](../../reference/hardware/PINS.md) and [WIRING](../../reference/hardware/WIRING.md). Avoid reserved pins owned by the LED matrix, UART, I²C or PWM. A production driver needs hardware initialization, ownership/locking, I/O bounds and electrical safety as well as VFS registration.

If adopted as a supported feature, update [DEVICES](../../reference/devices/DEVICES.md) and [FILESYSTEM](../../reference/FILESYSTEM.md) in both languages.

**Validation note:** The standalone driver C module has passed syntax checks; full Pico SDK compilation and physical-device testing remain unverified.

---

[Development index](../README.md) | [Syscall exercise](../syscalls/SYSCALL-DEVELOPMENT.md) | [日本語](../../../ja/development/drivers/DRIVER-DEVELOPMENT.md)
