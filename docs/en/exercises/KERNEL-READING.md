# Kernel reading exercises

Use these review questions alongside the [Kernel reading guide](../development/kernel/KERNEL-READING-GUIDE.md).

## Review questions

1. Why is program registration different from launching a task? Why is an idle task necessary?
2. Distinguish program ID, PID and slot. When is the Flash image copied to RAM?
3. How do the 1 ms SysTick and usual 10-tick quantum interact with PendSV? What happens if no user task is READY?
4. What do the hardware exception frame, PSP, MSP and r4–r11 each do?
5. How does `svc #0` enter the kernel and how are arguments/results passed? Where is the user buffer checked?
6. How do `/tmp`, `/proc` and `/dev` share `open/read/write/close`?
7. Why does a reader block? How are pipe EOF and reference counts related?
8. What belongs in device-specific code versus the VFS or `ioctl()` interface?
9. How are Wi-Fi/lwIP, built-in Telnet, and user-process sockets separated?

## Advanced and final exercises

Match the frame built by `spawn_in_slot()` to the restore sequence in `isr_pendsv`. Trace `device_rw_tty()` → `os_schedule()` → `wake_readers()` → `task_unblock()` and explain `wait_frame_ready` and `t->sp[8]`. Compare `SYS_SLEEP` with `SYS_YIELD`.

Trace a complete `hello` execution: the shell's `spawn()` call, task-slot allocation and initial context, PendSV scheduling, `write()` through VFS/TTY, and cleanup on exit. Next, draw the FD and pipe flow for `echo hello | cat`. Explain each transition with the names of the functions responsible, without modifying the kernel.

[Exercise index](README.md) | [日本語](../../ja/exercises/KERNEL-READING.md)
