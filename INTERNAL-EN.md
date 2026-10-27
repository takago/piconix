# Internal architecture

[日本語](INTERNAL-JP.md) | [Back to overview](README-EN.md)

This guide explains **what runs where and which component is responsible**. See the [overview](README-EN.md) for commands and [Setup](SETUP-EN.md) for building.

## 1. Memory layout

| Start address | Region | Notes |
|---|---|---|
| `0x20000000` | Kernel | 96 KiB including SDK support |
| `0x20018000` | Slot 1 | 32 KiB |
| `0x20020000` | Slot 2 | 32 KiB |
| `0x20028000` | Slot 3 | 32 KiB |
| `0x20030000` | Slot 4 | 32 KiB |
| `0x20038000` | Slot 5 | 32 KiB |

The last 4 KiB in each application slot is reserved for a stack. Applications are compiled as position-independent code (PIC); one image per program is embedded in flash and copied into a free RAM slot when started. There is no ELF loader, dynamic linking, or address-space isolation.

## 2. Boot and task switching

`src/main.c` initializes the kernel, registers applications, and starts Shell. `src/kernel.c` manages task state and scheduling. SysTick and PendSV provide preemptive switching, with exception handling in `src/context.S`.

Task states include `READY`, `SLEEPING`, `BLOCKED`, and `DEAD`. `spawn` chooses a free slot, and `waitpid` waits for or collects exit status. With all five slots occupied, no further application can start.

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
| `/proc/tasks` | Task listing (used by `ps`) |
| `/proc/uptime` and related paths | Uptime, interrupts, and other counters |
| `/tmp/name` | Simple volatile RAM files |

Descriptors belong to individual tasks. `dup2` copies descriptor state; **it does not implement POSIX shared file offsets**. Shell implements `<`, `>`, and `>>`. Redirection to devices does not use regular-file create/truncate flags. Pipes, shell quoting, and `2>` are not available.

## 7. Source map

- `src/kernel.c`: scheduler, SVC, descriptors, devices, RAM files
- `src/context.S`: exceptions and context switching
- `src/main.c`: boot and registration
- `ram_apps/shell.c`: parsing and redirection
- `ram_apps/slot.ld`: PIC image test link layout
- `CMakeLists.txt`: embedded application images and 32 KiB constraints

Next: [Application programming](PROGRAMMING-EN.md).

### PIC implementation

All applications are compiled with `-fPIC -Os`. The build links the same object at both `0x20018000` and `0x20020000` and checks that the resulting raw binaries are identical. Unresolved symbols, GOT sections, or dynamic relocations cause a build failure. On spawn, the single image stored in flash is copied to a free slot. This check is essential; arbitrary C code is not automatically relocatable.

**Note**: A full ARM GCC/Pico SDK build and RP2040 hardware run are still required. A self-launching Shell command and a second UART-connected Shell are not implemented merely by enabling PIC.

## Simplified terminal assignment (`getty`)

`getty /dev/uart1 sh &` launches a second shell attached to UART1. The kernel keeps shell-owner and foreground PIDs for USB and UART1 independently. `getty` assigns standard descriptors 0, 1 and 2 and the terminal affiliation at startup. Descendants inherit both, while ordinary input/output redirection does not change affiliation. Ctrl+C on each terminal terminates only its foreground task. No POSIX signals, sessions, process groups, login authentication or TTY mode switching are implemented. Hardware validation is still required.

### Terminal reporting and input ownership

`tty` retrieves the caller’s terminal via `SYS_GET_TTY`. The `TTY` column in `/proc/tasks` (and `ps`) reports each task’s terminal. While a terminal is owned by an interactive task, reads from other tasks are rejected; UART byte-stream reads remain available when it is unowned. Ctrl+C on UART1 discards pending received bytes and terminates the foreground task. This is not POSIX signal or process-group support.

On UART1, Ctrl+D (`0x04`) becomes EOF (a zero-length `read()`) only while a terminal owner exists. Without an owner, raw `0x04` is preserved. Bytes before Ctrl+D are returned first, followed by EOF on the next read.

The UART1 shell stays active on Ctrl+D (EOF); use `logout` or `exit` to terminate it and print a session-close message.

## Startup arguments and crt0

The initial task stack holds argument strings and argv pointers. Registers r0/r1 pass argc/argv to the common crt0. The old SYS_GETARG (number 15) is retired and reserved; other syscall numbers remain unchanged.


## Trace storage and link-time RAM report

The kernel reserves a 16-KiB trace ring (1,024 fixed 16-byte events), recording task switches and SVC entry/exit. Events remain binary; a readout command is planned separately. The post-link report shows the kernel SRAM high-water mark, linker headroom and five reserved task slots. Headroom is not free heap space.

### Kernel tracing (16 KiB)

`trace on` clears and starts the binary ring; `trace off` stops recording; `trace show` stops recording and prints the retained events. `/dev/trace` provides read-only, fixed 16-byte event records. Trace controls use device-specific `ioctl()` commands; decoding stays in the `trace.c` user application.

```text
trace on
hello
trace off
trace show
```

Events include task switches and SVC entry/exit. Timestamps use SysTick milliseconds since boot.
