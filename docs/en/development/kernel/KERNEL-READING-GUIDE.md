# PicoNIX Kernel Reading Guide

[Documentation](../../../DOCS.md) | [Kernel development](KERNEL-DEVELOPMENT.md) | [Function index (Japanese)](../../../FUNCTIONS.md) | [日本語](../../../ja/development/kernel/KERNEL-READING-GUIDE.md)

This guide explains **how to read the current PicoNIX kernel source**, rather than how to extend it. Follow one path through the system: **boot → task creation → scheduling → context switching → system calls → files → IPC → devices → networking (optional)**. Read one function and its callers and callees at a time. For modifications, use the [kernel development guide](KERNEL-DEVELOPMENT.md).

Before starting, read [Architecture](../../reference/ARCHITECTURE.md), [Memory layout](../../reference/MEMORY-LAYOUT.md), and [Differences from Linux](../../reference/DIFFERENCES.md). PicoNIX uses fixed-size task slots and does **not** implement the full process isolation or virtual memory of Linux. Use `rg -n 'function_name\(' kernel userland` to locate calls and definitions; line numbers can change as code is edited.

| Step | Topic | Read in this order | Check your understanding |
|---|---|---|---|
| 1 | Boot | `kernel/main.c`: `main()` → `os_init()` → `register_image()` → `net_init()` → `os_init_idle()` → `os_start_program(2)` → `os_start()`; also `idle_task()` | Why is program registration different from launching a task? Why is an idle task necessary? |
| 2 | Task creation and RAM | `include/piconix.h` constants → `os_register_program()` → `os_register_ram_image()` → `free_program_slot()` → `spawn_in_slot()` → `os_start_program()` in `kernel/kernel.c` | Distinguish program ID, PID and slot. When is the Flash image copied to RAM? |
| 3 | Scheduling | `os_start()` → `isr_systick()` → `pend()` → `os_schedule()`; inspect task states and `wake_readers()` / `wake_waiters()` / `wake_pipes()` | How do the 1 ms SysTick and usual 10-tick quantum interact with PendSV? What happens if no user task is READY? |
| 4 | Context switching | `kernel/context.S`: `isr_pendsv` → `kernel/kernel.c`: `os_schedule()` → return to `isr_pendsv` | What do the hardware exception frame, PSP, MSP and r4–r11 each do? |
| 5 | System calls | An application using `write()` → `userland/lib/syscalls.c`: `write()`, `call()` → `include/piconix_syscall.h` → `kernel/context.S`: `isr_svcall` → `kernel/kernel.c`: `os_svc_dispatch()` | How does `svc #0` enter the kernel and how are arguments/results passed? Where is the user buffer checked? |
| 6 | Files and FDs | `fd_set()` → `fd_release()` → `vfs_open()` → `vfs_rw()` → `tmp_open()` / `tmp_rw()` → `device_rw_proc()` / `device_rw_tty()` | How do `/tmp`, `/proc` and `/dev` share `open/read/write/close`? |
| 7 | TTY and pipes | `tty_input_char()` → `tty_enqueue()` → `tty_dequeue()` → `device_rw_tty()` → `wake_readers()`; then `pipe_ref()` → `pipe_transfer()` → `wake_pipes()` → FD helpers → `SYS_PIPE` / `SYS_DUP2` / `SYS_CLOSE` → shell pipeline handling | Why does a reader block? How are pipe EOF and reference counts related? |
| 8 | Devices | VFS device dispatch in `kernel/kernel.c` → `kernel/gpio.c` → `fb.c` → `adc.c` → `i2c.c` → `pwm.c` | What belongs in device-specific code versus the VFS or `ioctl()` interface? |
| 9 (optional) | Networking (Pico W) | `kernel/main.c`: `net_init()` → `kernel/net.c`: `net_init()` / `net_poll()` → `kernel/net_socket.c` → `kernel/net_service_telnet.c` / `kernel/net_service_remote_devctl.c` → `userland/apps/httpd.c` | How are Wi-Fi/lwIP, built-in Telnet, and user-process sockets separated? |

For step 2, try `ps`, `mem -s kernel`, and `mem -s sh`. For step 5, use `trace run hello` and follow this path:

```text
application → write() → call(SYS_WRITE, ...) → svc #0
            → isr_svcall → os_svc_dispatch() → vfs_rw() → TTY
```

For step 6, try `cat /proc/uptime`, `ls /dev`, and `lsof`. In step 8, follow VFS device dispatch before inspecting hardware backends; use the [device I/O lab](../../literacy/DEVICE-IO.md). Step 9 is a separate, optional Pico W topic; use the [network programming guide](../apps/c/NETWORK-PROGRAMMING.md) and distinguish kernel services from user-space `httpd`. For step 7, run:

```sh
echo hello | grep hello | cat
```

**Final exercise:** trace a complete `hello` execution: the shell's `spawn()` call, task-slot allocation and initial context, PendSV scheduling, `write()` through VFS/TTY, and cleanup on exit. Next, draw the FD and pipe flow for `echo hello | cat`. Explain each transition with the names of the functions responsible, without modifying the kernel.

Further reading: [Internals](../../reference/INTERNAL.md), [Syscalls](../../reference/SYSCALLS.md), [Trace](../../literacy/TRACE.md), [Memory inspection](../../literacy/MEM.md), [TTY](../../literacy/TTY.md), [Driver development](../drivers/DRIVER-DEVELOPMENT.md), [System call development](../syscalls/SYSCALL-DEVELOPMENT.md), and the [function index](../../../FUNCTIONS.md).
