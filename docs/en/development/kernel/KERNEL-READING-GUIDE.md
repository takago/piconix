# PicoNIX Kernel Source and Call-Path Reference

*Development guide*

This reference maps kernel functions to source files and shows the call paths of the current implementation. For source modifications, see the [kernel development guide](KERNEL-DEVELOPMENT.md).

Related specifications: [Architecture](../../reference/ARCHITECTURE.md), [Memory layout](../../reference/MEMORY-LAYOUT.md), and [Differences from Linux](../../reference/DIFFERENCES.md). PicoNIX uses fixed-size task slots and does **not** implement the full process isolation or virtual memory of Linux. Use `rg -n 'function_name\(' kernel userland` to locate calls and definitions; line numbers can change as code is edited.

| Subsystem | Function path and source location |
|---|---|
| Boot | `kernel/main.c`: `main()` → `os_init()` → `register_image()` → `net_init()` → `os_init_idle()` → `os_start_program(2)` → `os_start()`; also `idle_task()` |
| Task creation and RAM | `include/piconix.h` constants → `os_register_program()` → `os_register_ram_image()` → `free_program_slot()` → `spawn_in_slot()` → `os_start_program()` in `kernel/kernel.c` |
| Scheduling | `os_start()` → `isr_systick()` → `pend()` → `os_schedule()`; inspect task states and `wake_readers()` / `wake_waiters()` / `wake_pipes()` |
| Context switching | `kernel/context.S`: `isr_pendsv` → `kernel/kernel.c`: `os_schedule()` → return to `isr_pendsv` |
| System calls | An application using `write()` → `userland/lib/syscalls.c`: `write()`, `call()` → `include/piconix_syscall.h` → `kernel/context.S`: `isr_svcall` → `kernel/kernel.c`: `os_svc_dispatch()` |
| Files and FDs | `fd_set()` → `fd_release()` → `vfs_open()` → `vfs_rw()` → `tmp_open()` / `tmp_rw()` → `device_rw_proc()` / `device_rw_tty()` |
| TTY and pipes | `tty_input_char()` → `tty_enqueue()` → `tty_dequeue()` → `device_rw_tty()` → `wake_readers()`; then `pipe_ref()` → `pipe_transfer()` → `wake_pipes()` → FD helpers → `SYS_PIPE` / `SYS_DUP2` / `SYS_CLOSE` → shell pipeline handling |
| Devices | VFS device dispatch in `kernel/kernel.c` → `kernel/gpio.c` → `fb.c` → `adc.c` → `i2c.c` → `pwm.c` |
| Networking (Pico W) | `kernel/main.c`: `net_init()` → `kernel/net.c`: `net_init()` / `net_poll()` → `kernel/net_socket.c` → `kernel/net_service_telnet.c` / `kernel/net_service_remote_devctl.c` → `userland/apps/httpd.c` |


## Source map by subsystem

Assume prior knowledge of UNIX processes, FDs, syscalls and C. The following entries identify PicoNIX-specific entry points, state changes, and return paths. The [function index](../FUNCTIONS.md) helps locate definitions; line numbers may shift.

### Boot, image registration and task slots

Start with `kernel/main.c` and follow registration of flash-resident application images, then `spawn_in_slot()` in `kernel/kernel.c`. Distinguish *registering an image* from *loading and starting a task*. Inspect `OS_TASK_BYTES` and the fixed memory map in [Memory layout](../../reference/MEMORY-LAYOUT.md). Observe processes with `ps`, and use `mem -s kernel` / `mem -s sh` to inspect symbols.

### Tick, scheduling and context switch

Follow `isr_systick()` → `pend()` → `os_schedule()`, then `isr_pendsv` in `kernel/context.S`. Identify what is saved on each task's PSP and what runs on the shared MSP. Trace task states and the READY/BLOCKED transitions; do not assume that syscall handlers retain suspended C call frames.

### Syscall entry and return

Use `trace run hello`, then connect a visible `write` event to `write()` and `call()` in `userland/lib/syscalls.c`, `isr_svcall` in `context.S`, and `os_svc_dispatch()` in `kernel.c`. Follow the argument validation and result path. Consult [Syscalls](../../reference/SYSCALLS.md) for the PicoNIX ABI.

### VFS, FD, terminal and pipe

Compare `cat /proc/uptime`, `ls /dev`, and `lsof`. Follow FD lookup and VFS dispatch to the RAM file, `/proc`, or device backend. For pipelines, try `echo hello | grep hello | cat`, then locate shell pipe construction, `SYS_PIPE`, `SYS_DUP2`, `pipe_transfer()` and wake-up handling. See [Pipes](../../reference/commands/PIPE.md) and [TTY](../../reference/commands/TTY.md).

Screen handling is implemented in `kernel/screen.c` / `kernel/screen.h`: `screen_byte()` parses ANSI output, `screen_read()` serializes PTY screen state, and `screen_key_decode()` converts key names. `screen_cdc_poll()` in `screen.c` now owns incremental CDC repaint generation. `kernel.c` retains FD ownership, TTY input queues, the USB TX queue, CDC key-action dispatch, TTY input routing, and console/shell switching. CDC escape-sequence parsing is in `screen.c`.

### Device I/O

Begin at VFS device dispatch in `kernel/kernel.c`, then follow a selected backend such as `kernel/gpio.c`. Separate path/device identification from the read/write/ioctl operation and hardware-specific code. Consult [Device reference](../../reference/devices/README.md) and [Driver development](../drivers/DRIVER-DEVELOPMENT.md).

### Networking (Pico W only)

Follow `net_init()` and `net_poll()` in `kernel/net.c`, the socket adaptation in `kernel/net_socket.c`, and the kernel's Telnet/remote-control services. Compare these with the **user-space** `userland/apps/httpd.c`. Read [Kernel networking development](../network/NETWORK-DEVELOPMENT.md) and [Socket programming](../apps/c/NETWORK-PROGRAMMING.md). The network subsystem is available only on Pico W.


## Deep dive: CPU modes, stacks, SVC and PendSV in the source

Distinguish Cortex-M0+ architectural behavior from PicoNIX's choices. Follow the named symbols in `kernel/context.S` and `kernel/kernel.c` rather than relying on fixed line numbers.

### 1. Thread/Handler mode and privilege

The processor runs ordinary application code in **Thread mode**, and SVC, PendSV, SysTick and interrupt handlers in **Handler mode**. Mode and privilege are different concepts: Handler mode is always privileged; Thread mode may be privileged or unprivileged. PicoNIX user tasks run in privileged Thread mode and use the PSP. This does **not** provide Linux-style hardware-enforced isolation between user and kernel code.

| Execution | Mode | Stack | Source |
|---|---|---|---|
| PicoNIX task | Thread | Per-task PSP | `spawn_in_slot()` in `kernel/kernel.c` |
| System call | Handler (SVC) | Shared MSP | `isr_svcall` in `kernel/context.S` |
| Context switch | Handler (PendSV) | Shared MSP | `isr_pendsv` in `kernel/context.S` |
| Tick/interrupt | Handler | Shared MSP | `isr_systick()` in `kernel/kernel.c` |

Handler mode does not disable interrupts: a higher-priority unmasked exception may preempt the current handler. It also provides automatic exception priority and return handling, not just stack selection.

### 2. PSP, MSP and exception frames

**PSP and MSP are processor registers, not memory regions.** Each task has a saved PSP in `task_t.sp`; handlers share the MSP stack. `spawn_in_slot()` constructs an initial task frame, while `os_init_idle()` prepares the idle task frame.

When an exception interrupts Thread mode using PSP, hardware first stacks `r0–r3, r12, lr, pc, xPSR` **on that PSP stack**, then runs the handler using MSP. The saved user registers are not automatically put on MSP.

```text
Per-task PSP stack                     Shared MSP stack
  r4–r11 (saved by PendSV)              Temporary call frames for handlers
  r0–r3, r12, lr, pc, xPSR              and kernel functions
  (hardware exception frame)
```

`isr_pendsv` saves `r4–r11` on the old task's PSP stack and calls `os_schedule(oldsp)` to save that pointer and choose a new task. It restores the next task's registers and PSP, then uses exception return `0xFFFFFFFD` to resume Thread mode with PSP. The CPU has one PSP register, but each task retains its own saved value.

### 3. SVC versus PendSV

**SVC is the system-call entry.** `call()` in `userland/lib/syscalls.c` places the syscall number in `r0`, arguments in `r1–r3`, and executes `svc #0`. `isr_svcall` passes the exception frame to `os_svc_dispatch()`, which places the result in the saved `r0`.

**PendSV is a pendable exception used for scheduling.** `pend()` in `kernel/kernel.c` sets the ICSR `PENDSVSET` bit. `isr_systick()` updates time every 1 ms and requests PendSV normally every 10 ticks or when a sleeping task wakes. `SYS_SLEEP` and `SYS_YIELD` also request it. PendSV itself does not mark tasks `BLOCKED`.

With PendSV at the lowest priority, a request made inside SVC or another higher-priority handler normally stays pending until that handler completes. This enables reuse of the shared MSP after the current handler has unwound.

```text
Task A (Thread, PSP A)
  → svc #0
    → isr_svcall (Handler, MSP)
      → os_svc_dispatch() sets waiting state and requests PendSV
    → SVC handler completes
  → isr_pendsv (Handler, MSP)
    → os_schedule() chooses PSP B
  → Task B (Thread, PSP B)
```

### 4. Sleep and blocking I/O without retaining kernel call frames

PicoNIX **does not retain a suspended kernel C call chain on MSP for each blocked task**. MSP memory is not freed; rather, after handlers finish, its temporary stack space can be reused.

- **`SYS_SLEEP`** sets `task_t.wake` and `state=SLEEPING` in `os_svc_dispatch()`, then requests PendSV. `isr_systick()` eventually marks the task `READY`. The syscall's user-side exception-return state is retained; execution does not resume inside a suspended kernel `sleep()` C function.
- **Blocking TTY `read()`** uses `device_rw_tty()` to save `wait_dev`, `wait_fd`, `wait_buf`, and `wait_size`; it sets `state=BLOCKED` when input is unavailable. The kernel C call returns, but the user task remains blocked.
- **Completed context frame**: `os_schedule()` saves the old PSP to `task_t.sp` and sets `wait_frame_ready=1`. `wake_readers()` checks this flag, accounting for the possibility that data arrives before PendSV finishes saving the frame.
- **Completion**: `wake_readers()` copies data into the saved `wait_buf` and calls `task_unblock(t, n)`. The latter writes the result to **`t->sp[8]` (saved `r0`)** and marks the task `READY`. `wake_pipes()` and `wake_waiters()` similarly complete other blocked operations.

```text
read(fd, buf, size) → SVC
  → no input: save buf/size, set BLOCKED, request PendSV
  → SVC C code returns; shared MSP becomes reusable
  → input arrives: copy data to buf and byte count to saved r0
  → task becomes READY; exception return resumes its user code
  → read() appears to have returned normally
```

**Do not confuse the data buffer with the return-value slot.** Data goes into the user buffer passed to `read()`; the byte count goes into saved `r0` on PSP. The buffer must remain valid until completion. This does not mean every device operation blocks asynchronously; some drivers may perform short synchronous operations.

### 5. Comparison with Research UNIX

In Research UNIX V5/V6, a process can call `sleep()` **inside the kernel**, switch away with its kernel call frames preserved on a **per-process kernel stack**, and later continue from that point. This is useful even with a generally non-preemptible kernel, because the process may voluntarily block while inside a syscall.

PicoNIX instead retains the wait condition and the **user-side syscall-return context**, without keeping the kernel C call chain suspended. Both implementations can present an ordinary blocking `read()` interface to applications, but they manage the waiting process differently.


Useful observation commands include `ps`, `mem -s kernel`, `mem -s sh` and `trace run hello`. The syscall path for `write()` is:

```text
application → write() → call(SYS_WRITE, ...) → svc #0
            → isr_svcall → os_svc_dispatch() → vfs_rw() → TTY
```

Additional VFS observations: `cat /proc/uptime`, `ls /dev`, and `lsof`. The [device reference](../../reference/devices/README.md) covers hardware backends. Pico W networking is described in [kernel networking](../network/NETWORK-DEVELOPMENT.md) and [user socket programming](../apps/c/NETWORK-PROGRAMMING.md); the kernel services differ from user-space `httpd`. A shell pipeline example is:

```sh
echo hello | grep hello | cat
```


Further reading: [Internals](../../reference/INTERNAL.md), [Syscalls](../../reference/SYSCALLS.md), [Trace](../../reference/commands/TRACE.md), [Memory inspection](../../reference/commands/MEM.md), [TTY](../../reference/commands/TTY.md), [Driver development](../drivers/DRIVER-DEVELOPMENT.md), [System call development](../syscalls/SYSCALL-DEVELOPMENT.md), and the [function index](../FUNCTIONS.md).

---

**Related links:** [Development guide index](../README.md) | [Documentation](../../README.md) | [日本語](../../../ja/development/kernel/KERNEL-READING-GUIDE.md)

## CDC virtual-console repaint

`screen_cdc_begin_repaint()` starts a repaint, and `screen_cdc_poll()` produces at most 96 bytes per idle poll via a callback into `kernel.c`'s USB TX queue. When the queue is full, rendering resumes on a later poll. The active screen and dual-pane display state are stored in `cdc_active` and `cdc_dual_mode`. `screen_cdc_key_feed()` and `screen_cdc_key_timeout()` parse CDC control keys and preserve incomplete ESC sequences. `kernel.c` routes ordinary bytes to TTY input, performs console switching, and retains lazy shell startup.


For screen-module internals, see [Screen Management](SCREEN-MANAGEMENT.md).
