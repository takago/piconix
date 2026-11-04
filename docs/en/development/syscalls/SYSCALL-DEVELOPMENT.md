# System Call Development

*Development guide*

## Before you start

**Prerequisites:** Call `write()` from a C app and observe the transition to the kernel using [Tracing](../../guides/TRACE.md). This guide adds an interface to the kernel; it is not a guide to merely using existing APIs.


[Kernel development](../kernel/KERNEL-DEVELOPMENT.md) | [System call reference](../../reference/SYSCALLS.md) | [日本語](../../../ja/development/syscalls/SYSCALL-DEVELOPMENT.md)

This guide explains how to **add** a system call. For using existing calls, refer to the C programming and system call reference guides.

## 1. Adding a system call

### 1.1 Allocate a number and define the ABI

`include/piconix_syscall.h` defines `PICONIX_SYSCALL_TABLE(X)` with numbers 1–39. **Numbers 11 and 15 are reserved.** Never reorder or reuse existing numbers. Append a new explicitly numbered entry rather than shifting existing values. The user wrappers in `userland/lib/syscalls.c` use the shared `SYS_*` constants from that header. Use the single common table for new numbers to preserve the ABI.

The calling convention is `r0` = syscall number, `r1`–`r3` = arguments, `svc #0`, and return value in `r0`. Use a pointer to a validated structure if more argument fields are needed. Errors normally return negative `OS_E*` values rather than setting `errno`. See [SYSCALLS.md](../../reference/SYSCALLS.md).

### 1.2 Implement the kernel handler

`os_svc_dispatch(uint32_t *f)` in `kernel/kernel.c` dispatches on `switch(num)` and sets `result`. Follow simple existing cases such as `SYS_FLASH_ID` or `SYS_GETPID`.

```c
/* Schematic placement only; do not paste without defining the symbols. */
case SYS_EXAMPLE:
    result = example_kernel_operation(a);
    break;
```

`a`, `b` and `c` correspond to `r1`–`r3`. Before dereferencing user pointers, validate **length, address range, termination and integer overflow** using existing patterns such as `in_task_range()` and path validation. Never dereference an arbitrary user-supplied address. Review interrupt protection and scheduling rules when modifying shared state or blocking a task.

### 1.3 Export the C wrapper

`call(n, a, b, c)` in `userland/lib/syscalls.c` issues `svc #0`. Add a C wrapper and a matching prototype to the appropriate public header, such as `userland/lib/unistd.h`. Keep parameter types, register order and error results consistent with the kernel. Update both language versions of [SYSCALLS.md](../../reference/SYSCALLS.md) and [LIBC.md](../apps/c/LIBC.md).

### 1.4 Test the ABI

- Existing numbers and reserved entries remain unchanged; the new number is unique
- Zero, boundary, invalid-FD, invalid-pointer and oversized inputs return defined errors
- Failures do not corrupt task state, FDs or buffers
- Wrapper argument positions match the dispatcher
- SVC events appear in `trace` where relevant

---

**Related links:** [Development guide index](../README.md) | [Part C textbook](../../learn/C/README.md)
