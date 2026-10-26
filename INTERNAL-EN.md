# Internal architecture

[日本語](INTERNAL-JP.md) | [Back to overview](README-EN.md)

This guide explains **what runs where and which component is responsible**. See the [overview](README-EN.md) for commands and [Setup](SETUP-EN.md) for building.

## 1. Memory layout

| Start address | Region | Notes |
|---|---|---|
| `0x20000000` | Kernel | 64 KiB including SDK support |
| `0x20010000` | Slot 1 | Shell, 64 KiB |
| `0x20020000` | Slot 2 | Blink, 64 KiB |
| `0x20030000` | Slot 3 | Omikuji or Python, 64 KiB |

The last 4 KiB in each application slot is reserved for a stack. Applications are linked to fixed addresses by `ram_apps/slot.ld`, embedded into flash, and copied into RAM. There is no ELF loader, dynamic linking, or address-space isolation.

## 2. Boot and task switching

`src/main.c` initializes the kernel, registers applications, and starts Shell. `src/kernel.c` manages task state and scheduling. SysTick and PendSV provide preemptive switching, with exception handling in `src/context.S`.

Task states include `READY`, `SLEEPING`, `BLOCKED`, and `DEAD`. `spawn` starts an application in an assigned slot, and `waitpid` waits for or collects its exit status. Omikuji and Python share a physical slot and cannot run concurrently.

## 3. Kernel and Shell responsibilities

| Operation | Primary owner |
|---|---|
| Scheduling, creating and terminating tasks | Kernel |
| I/O, devices, descriptors, and RAM files | Kernel |
| Parsing commands, `cd`, resolving relative paths | Shell |
| Parsing `&`, selecting the foreground task | Shell |
| Detecting `Ctrl+C` and terminating its target | Kernel |

The current directory is Shell-local. The kernel generally handles absolute virtual paths.

## 4. System calls

RAM applications do not link directly against kernel functions. They invoke **SVC** with a syscall number in `r0`, arguments in `r1`–`r3`, and the result in `r0`. Numbers must agree between `SYS_*` definitions in `src/kernel.c` and the applications.

Operations include `spawn`/`kill`/`waitpid`, `sleep`/`yield`, `open`/`read`/`write`/`close`/`dup2`, and `ioctl`. Avoid changing syscall numbers without preserving compatibility.

## 5. Foreground/background jobs and signals

For foreground `blink`, Shell registers its PID with the kernel through `SYS_SET_FG`, then waits with `waitpid`. With `blink &`, Shell returns to its prompt without waiting. This is not general background execution for every Shell built-in.

**There is no generic signal mechanism.** Kernel console input handles `Ctrl+C` by terminating the registered foreground task directly. `kill PID` invokes `SYS_KILL` for direct task termination. There is no `SIGINT`/`SIGTERM` delivery, handler registration, or UNIX process-group support. UNIX-like status numbers do not imply POSIX signal semantics.

## 6. Files and devices

| Path | Purpose |
|---|---|
| `/dev/console` | Console I/O |
| `/dev/uart1` | UART1 I/O |
| `/dev/rand` | Pseudorandom input |
| `/dev/gpio/N`, `N.dir`, `N.pull` | GPIO value, direction, and pull configuration |
| `/sys/proc/tasks` | Task listing (used by `ps`) |
| `/sys/proc/uptime` and related paths | Uptime, interrupts, and other counters |
| `/tmp/name` | Simple volatile RAM files |

Descriptors belong to individual tasks. `dup2` copies descriptor state; **it does not implement POSIX shared file offsets**. Shell implements `<`, `>`, and `>>`. Redirection to devices does not use regular-file create/truncate flags. Pipes, shell quoting, and `2>` are not available.

## 7. Source map

- `src/kernel.c`: scheduler, SVC, descriptors, devices, RAM files
- `src/context.S`: exceptions and context switching
- `src/main.c`: boot and registration
- `ram_apps/shell.c`: parsing and redirection
- `ram_apps/slot.ld`: fixed RAM layout
- `CMakeLists.txt`: embedded application images and 64 KiB constraints

Next: [Application programming](PROGRAMMING-EN.md).
