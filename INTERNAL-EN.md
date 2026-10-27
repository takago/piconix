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

The kernel reserves a 16-KiB trace ring (512 fixed 32-byte events), recording task switches and SVC entry/exit. Events remain binary; a readout command is planned separately. The post-link report shows the kernel SRAM high-water mark, linker headroom and five reserved task slots. Headroom is not free heap space.

### Kernel tracing (16 KiB)

`trace on` clears and starts the binary ring; `trace off` stops recording; `trace show` stops recording and prints the retained events. `/dev/trace` provides read-only, fixed 32-byte event records. Trace controls use device-specific `ioctl()` commands; decoding stays in the `trace.c` user application.

```text
trace on
hello
trace off
trace show
```

Events include task switches and SVC entry/exit. Timestamps use SysTick milliseconds since boot.

- `/bin`: read-only list of registered external commands. `spawn` supports `argv[]` with up to eight arguments and 128 bytes of strings.

`trace show` displays Time(ms), PID, Command, Event and Detail columns. Command is resolved from the program ID captured at event time via `/bin`, so exited tasks retain their names. For switches, PID and Command refer to the destination task. Binary records remain 16 bytes.

Trace details: [`TRACE-EN.md`](TRACE-EN.md) / [`TRACE-JP.md`](TRACE-JP.md)


## waitpid / Shell built-ins

`waitpid(pid, status, 0)` blocks until the child exits; `WNOHANG` is nonblocking. Child task slots are released immediately, and up to five PID-only completion notifications are retained. Exit status is not retained. Orphan exit notifications are discarded. `ls /bin` lists built-ins after `---- sh built-in commands ----`.


## External ps, cat, watch and interrupt counters

`ps` and `cat` are standalone PIC applications (`/bin/ps`, `/bin/cat`), no longer Shell builtins. `watch` is a standalone `/bin/watch` application.

```text
ps
cat /proc/tasks
watch ps
watch 1 ps
watch 1 cat /proc/interrupts
watch 2 cat /proc/memory
```

`watch [seconds] command [args...]` repeatedly executes an external program. The interval is 1–60 seconds, default 1 second. The terminal is cleared using ANSI escapes. `watch` and its child occupy one task slot each. `ls` and `echo` are external commands and can run under `watch`. Ctrl+C only terminates the terminal's foreground PID, not a process group; a child may remain briefly if interrupted while running. Use short-lived commands.

`SysTick` in `/proc/interrupts` counts actual 1-ms interrupts. `uart1` counts entries into the UART1 IRQ handler. UART0-console builds report UART0 IRQs as `console`. USB CDC builds omit `console` because USB IRQs are not counted. `PendSV` and `SVC` count exception handler entries.


## External shell commands and working directories

`pwd`, `ls`, `rm`, `echo`, `kill`, and `tty` are separate `/bin` PIC applications. The `pid` command was removed; `getpid()` remains available. Shell built-ins are `help`, `cd`, `exit`, `logout`, and `getty`.

The kernel maintains one working directory per task, inherited through `spawn()`. `chdir()` changes the calling task's directory; `getcwd()` returns it. `open()` and `unlink()` resolve relative paths. After `cd /tmp`, `pwd`, `ls`, `cat name`, and `rm name` use that directory.

`ls /bin` lists external applications, followed by the Shell built-ins under their own heading. `watch 1 ls /tmp` and `watch 1 pwd` are now supported.

### PWM and reserved GPIO pins (2026-10-27)

GP18 and GP19 are dedicated PWM outputs, sharing a default 1 kHz frequency, with initial duties 50% and 0%. `/dev/pwm/freq` accepts 8–100000 Hz, and `/dev/pwm/18` and `/dev/pwm/19` accept 0–100 percent via decimal text reads/writes. GPIO listings and access exclude GP18, GP19 and ADC-reserved GP26–28.


### Pin configuration and drivers (2026-10)

Edit `GPn_MODE` in `src/pins.h` and rebuild to change exposed `/dev/gpio`, `/dev/adc`, and `/dev/pwm` nodes and hardware initialization. Hardware drivers are in `src/gpio.c`, `src/adc.c`, and `src/pwm.c`; descriptor management remains in `src/kernel.c`. PWM defaults to 1 kHz and 0% duty on both GP18 and GP19. GP0/1/4/5 remain reserved for UART.
