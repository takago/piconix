# System call development: add and test `getuptime()`

This is a **source-level practical exercise**: add a new SVC that returns milliseconds since boot, expose it to a C application, build firmware and check the result. The existing `/proc/ticks` reports similar information; the new interface is intentional so you can learn the SVC path. First read the [Kernel Reading Guide](../kernel/KERNEL-READING-GUIDE.md) and [C application tutorial](../apps/c/PROGRAMMING.md).

## 1. Identify the data path and existing ABI

```text
labuptime.c → getuptime() → userland/lib/syscalls.c: call()
→ svc #0 → kernel/kernel.c: os_svc_dispatch() → ticks → r0
```

From the Linux PC in the repository root, inspect the current definitions:

```sh
grep -n 'SYS_SELECT\|SYS_GETUPTIME' include/piconix_syscall.h
grep -n 'case SYS_FLASH_ID\|case SYS_GETPID' kernel/kernel.c
```

In the checked source, 1–40 are defined; 11 and 15 are reserved. Number 40 belongs to `select()`. Choose unused number **41** for this exercise, but check conflicts again in a newer tree. The ABI passes the syscall number in r0, arguments in r1–r3, executes `svc #0`, and returns the result in r0.

## 2. Extend the common syscall table

Append an entry in `include/piconix_syscall.h`; preserve the trailing backslash on the preceding entry:

```c
    X(SYS_SHUTDOWN_TCP,  39, "shutdown") \
    X(SYS_SELECT,        40, "select") \
    X(SYS_GETUPTIME,    41, "getuptime")
```

Do **not** renumber existing calls. This is the shared source of names and numbers.

## 3. Implement the kernel handler

In the `switch(num)` within `os_svc_dispatch(uint32_t *f)` in `kernel/kernel.c`, add:

```c
case SYS_GETUPTIME:
    result = (int)ticks; /* 32-bit payload in r0 */
    break;
```

`ticks` is the existing 1 ms `uint32_t` counter; it wraps after about 49.7 days. The C wrapper interprets its bits as `unsigned`. There is no separate error channel for this deliberately simple no-argument syscall. For syscalls that receive user pointers, use `in_task_range()` and validate lengths before dereferencing. That pointer validation is unnecessary here because this syscall has no pointer arguments.

## 4. Publish a C wrapper and prototype

In `userland/lib/syscalls.c`:

```c
unsigned getuptime(void) {
    return (unsigned)call(SYS_GETUPTIME, 0, 0, 0);
}
```

In `userland/lib/unistd.h`:

```c
unsigned getuptime(void); /* ms since boot, wraps at 32 bits */
```

The handler, the shared number table and the wrapper must agree; adding only a header prototype does not create a syscall.

## 5. Register a test program

The example `labuptime` below compares the new syscall with a new `/dev/uptime` device. Apply the [driver exercise](../drivers/DRIVER-DEVELOPMENT.md) to enable the device. To test **only** the syscall, temporarily omit the `open()`/`read()` portion.

```c
/* Compare the new syscall with the new read-only device node. */
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

int main(void) {
    printf("getuptime: %u ms\n", getuptime());
    int fd = open("/dev/uptime", O_RDONLY);
    if (fd < 0) { printf("open: %d\n", fd); return 1; }
    char bytes[8];
    int n;
    while ((n = read(fd, bytes, sizeof bytes)) > 0) {
        if (write(1, bytes, (unsigned)n) < 0) break;
    }
    close(fd);
    return n < 0 ? 1 : 0;
}
```

Add `labuptime` to the existing `foreach(APP IN ITEMS ...)` in `CMakeLists.txt`. Add declarations and registration to `kernel/main.c`:

```c
extern const unsigned char piconix_labuptime_image_start[], piconix_labuptime_image_end[];
/* within main(), after existing registrations */
register_image(51,"labuptime",piconix_labuptime_image_start,piconix_labuptime_image_end);
```

51 was unused in the checked source. Check again if you have added other programs. The full app installation process is explained in [Programming](../apps/c/PROGRAMMING.md).

## 6. Apply and verify the lab on an isolated branch

**The obsolete `uptime_lab.patch` has been removed because it does not apply to this source tree.** Enter the changes from sections 2–5 and the driver development lab manually. Do not apply a patch from an earlier revision.

```sh
git switch -c exercise/uptime-lab
# Edit the files in this guide and the driver exercise.
git add -N kernel/uptime_device.c kernel/uptime_device.h userland/apps/labuptime.c
git diff --check
python3 tools/run_tests.py
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

The example is not part of the stock OS. Verify the SDK build and actual hardware behavior independently.

## 7. Observe and explain

After writing the UF2, in the PicoNIX shell:

```text
piconix# cat /proc/ticks
15327
piconix# labuptime
getuptime: 15410 ms
15.410 s
piconix# trace run labuptime
...
```

These are **illustrative outputs**, not captured hardware results. The values change. Use the [Trace guide](../../guides/TRACE.md) to find the new `getuptime` SVC. If the new program is missing, recheck CMake, image registration and flashing; if the firmware is old, the new syscall may return `OS_ENOSYS`.

**Questions:** Why is the number defined centrally? What happens after 32-bit wrap? What does `trace` show at the user/kernel boundary? Why does `/proc/ticks` require no new syscall?

## 8. Keep the references up to date

If this becomes a supported API, update [SYSCALLS](../../reference/SYSCALLS.md) and [LIBC](../apps/c/LIBC.md) in both languages, including signature, number, return semantics and wrap. Keep the exercise separate from normal development; switch back to your original branch afterward.

**Validation note:** The example patch was checked for application and its standalone C files for syntax. SDK firmware compilation and real-device execution were **not** performed here.

---

[Kernel development](../kernel/KERNEL-DEVELOPMENT.md) | [Device driver exercise](../drivers/DRIVER-DEVELOPMENT.md) | [日本語](../../../ja/development/syscalls/SYSCALL-DEVELOPMENT.md)
