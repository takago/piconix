# PicoNIX Kernel Tracing

*Usage guide*

PicoNIX records task switches, system-call entry/exit, and periodic timer events through `/dev/trace`. Use the `trace` command to inspect these events.

## Commands

```text
trace on         # clear the buffer and start recording (SVC functions)
trace on -a      # record selected functions outside SVC too
trace off        # stop recording
trace show       # stop recording and display all retained events
trace show 3     # stop recording and show events for PID 3
trace follow     # display events occurring after invocation
trace follow 3   # display new events for PID 3 after invocation
trace run hello  # start hello and show its trace after exit
trace run python test.py  # trace a Python script
trace run -a ping 127.0.0.1 # include selected asynchronous functions
trace run cat /tmp/test.txt  # include selected functions when built with instrumentation
```

**`trace show` and `trace follow` display events; they do not start a target program.** With only one terminal, `trace run hello` is a convenient first exercise because it starts the program and displays its trace after exit.

The PID argument is a decimal integer. **PID selection filters displayed records**, not the events captured by the kernel. Start with `trace on`, then run the program you want to observe. `show` stops tracing before reading the retained history; `follow` skips existing buffered records on startup and displays only events occurring afterward, without stopping capture.

For `SWITCH` records, the filter matches the **destination PID** (the event's `value`); for SVC records, it matches the event PID. `follow` runs in the foreground. Stop tracing from another terminal with `trace off` to drain the remaining records and exit, or interrupt the viewer with Ctrl+C. If the viewer falls behind, it reports overwritten records.

## Start and trace a command (`trace run`)

`trace run command [args...]` **starts a new program**, waits for it to exit, and then displays the trace records for its PID. You do not need to look up a PID in advance, and trace output will not interrupt the program's normal screen updates. A registered command, `/bin/name`, or a `/tmp/name` executable image can be specified, including `./name` from the current directory. Executable images in `/tmp` do not accept arguments.

```text
trace run hello
trace run ls
trace run python test.py
trace run /tmp/a.out
trace run ./a.out
```

The reader advances its `/dev/trace` cursor past existing events before launching the program, so earlier records are excluded. If tracing was off, this command enables it for the run and then disables it. If tracing was already enabled, **it leaves both the existing records and the tracing state intact**. Concurrent events from unrelated processes are excluded from the output by PID.

The 16 KiB (512-event) ring buffer is still used, located in dedicated RAM at `0x20038000`–`0x2003BFFF`. Long-running programs may overwrite their earliest records, including startup events. Only the directly spawned process is filtered; descendants are not automatically followed. Interactive programs must exit before the trace report is printed.

## Selected kernel function tracing

The default build records system calls, scheduling and periodic SysTick events. Build with
`-DPICONIX_KERNEL_FUNCTION_TRACE=ON` to include entry/exit events for selected kernel functions.
The default is OFF; CMake may cache a previous ON value.

```text
trace on              # record ordinary events + selected functions in SVC
trace on -a           # include selected functions outside SVC
trace show            # stop and display the recorded events
trace follow          # display new events as they arrive
trace run hello       # run a command; capture functions called inside SVC
trace run -a hello    # capture selected functions in all contexts
```

The `-a` switch changes the context filter only for selected kernel function events.
SVC function records use their calling process PID. Interrupt and asynchronous kernel records
have no PID and display a dash. `show PID` and `follow PID` exclude PID-less events.
`run -a` records all contexts but displays only the launched PID after the command exits;
use `on -a` and `follow` to inspect asynchronous records.

Coverage includes selected VFS, file-descriptor, RAM-file, TTY, ADC, I2C, PWM, GPIO,
framebuffer and TCP/UDP functions. ICMP device calls `net_echo_write`, `net_echo_read`,
and `net_echo_close` are included, but `net_capture_poll` and `echo_received` are not.
The existing record ABI is 32 bytes, with at most 16 bytes for function names, and the
shared 16 KiB ring stores 512 events. Function entries/exits normally appear as `name` / `name/`. **An immediately adjacent entry/exit for the same function, PID, and recorded millisecond** is condensed to a single `(name)` row at display time. A nested call, intervening event, or differing timestamp prevents condensation. The underlying records and buffer usage are unchanged. `trace follow` briefly holds at most one entry record for matching, then prints it separately when no further records are available.
Display indentation is approximate, especially across interrupts or overwritten events.
Compiler optimizations may inline calls. All-context recording increases runtime overhead.
The ring buffer is protected against interrupt-time re-entry, but real-device timing and
capacity must still be validated. The old `trace kernel` command has been removed.

## Event types and display

| Event | Meaning |
|---|---|
| `SWITCH` | Context switch |
| `read` / `read/` etc. | System-call entry and exit, using the syscall name |
| `TICKx1K` | One record for every 1,000 ticks of the 1ms SysTick |
| `vfs_rw` / `vfs_rw/` etc. | Entry/exit of selected kernel functions (experimental) |

The display contains `Time | PID | Command | TTY | Type | Function | Arg | Ret | Info`. The `Type` column distinguishes `SYSCALL`, `KERNEL`, `SWITCH`, `TIMER`, and `OTHER`. Kernel functions directly inside a syscall start at indentation depth 1; the syscall itself is depth 0. `Arg` carries the previously displayed target or recorded argument, not necessarily every function argument. `Ret` is the recorded syscall return value; internal kernel-function return values are not captured. `Info` contains auxiliary details such as I/O previews or task switches. Missing or inapplicable fields show `--`. The `exit` record is an OS termination event, not a conventional function return. The trace command's own system calls and related context switches are excluded from capture.

## Try it: observe system calls and task switches

```text
trace run hello
trace run ls
```

After each program exits, its trace is displayed. Compare `function` and `function/` records to see which system calls it made. `SWITCH` marks a context switch; filtering uses the destination PID.

For a longer capture, start with `trace on`, run another command, then inspect the result using `trace show` (which stops capture). On long runs, older events may be overwritten in the ring buffer.

## Hands-on exercises: SVC and asynchronous execution

These exercises assume a build with `PICONIX_KERNEL_FUNCTION_TRACE=ON`. An ordinary build still records system calls, SysTick events and task switches, but does not show `KERNEL` function records. Actual timestamps, PIDs and call sequences vary.

### Exercise 1: system calls and selected kernel functions

```text
piconix# trace run hello
piconix# trace run ls
piconix# trace run python test.py
```

Compare `SYSCALL` and `KERNEL` records to follow calls from a system call into selected VFS or driver functions. An entry is shown as the function name and an exit with a trailing `/`. Adjacent entry/exit records may be shown as one `(name)` row. Without `-a`, function records are limited to SVC execution. `trace run` displays only records associated with the launched PID.

### Exercise 2: ICMP device activity during ping

```text
piconix# trace run ping 127.0.0.1
```

Look for `open`, `write` and `read` calls involving `/dev/net/icmp`. In instrumented builds, `net_echo_write`, `net_echo_read` and `net_echo_close` may appear if actually called in SVC context. This does not trace every asynchronous stage of ICMP reception.

### Exercise 3: observe other contexts using two terminals

Use **two terminals**, such as USB CDC plus UART1 or Telnet.

Terminal A:

```text
piconix# trace on -a
piconix# trace follow
```

Terminal B:

```text
piconix# ping 127.0.0.1
piconix# ls
```

Look for `SYSCALL`, `SWITCH`, `TIMER` and `KERNEL` records in terminal A. The `-a` option widens capture of **selected instrumented kernel functions** beyond SVC. Asynchronous and interrupt-context function events have no PID and appear as `-`. Functions not selected or instrumented, such as `net_capture_poll` and `echo_received`, are not recorded. The periodic SysTick `TICKx1K` record is an ordinary event and does not depend on `-a`.

From another terminal, stop capture:

```text
piconix# trace off
```

`trace follow` drains the remaining records and exits. Note that `trace show` also stops capture before displaying history.

### Exercise 4: compare with `trace run -a`

```text
piconix# trace run -a ping 127.0.0.1
```

The `-a` option **records** selected functions in all contexts, but the resulting output **displays** records for the launched ping PID only. To inspect PID-less asynchronous functions, use `trace on -a` with `trace follow` as in exercise 3. Keep captures short: the 512-event ring buffer overwrites old records when full.

## Labs: files, devices, networking, and scheduling

Use `trace run` for short-lived programs and `trace on` plus `trace follow` from another terminal for long-running servers. `trace run` prints its report only after the target terminates. Stop a live capture using `trace off` from another terminal.

### Exercise 5: RAM files and VFS

Prepare a small RAM file first:

```text
piconix# echo hello > /tmp/test.txt
piconix# trace run cat /tmp/test.txt
```

Look for `open`, `read`, `write`, and `close`, and compare them to selected `KERNEL` calls such as `vfs_rw`. A compact `(in_task_range)` row represents consecutive entry and exit records at the same recorded time. The two underlying events remain in the buffer.

### Exercise 6: Python file operations and ADC

```text
piconix# trace run python test.py
```

The bundled `test.py` reads the on-chip temperature sensor five times. Python assignments and integer arithmetic run in user space and do not themselves generate `SYSCALL` events. Observe device-file `open`, `read`, `write`, and `close`, together with any waiting syscalls. Check the contents of `/tmp/test.py` when interpreting the result.

### Exercise 7: BH1750 and I2C ioctl

With a BH1750 connected, limit the program to one measurement:

```text
piconix# trace run bh1750 1
```

Observe `open` for `/dev/i2c/1`, `ioctl` for the transfer, and `close` during cleanup. In an instrumented build, look for `i2c_driver_transfer` when the corresponding SVC path runs. A sensor-free board cannot be expected to return a valid measurement. Without a count argument, `bh1750` measures continuously, so supply a count when using `trace run`.

### Exercise 8: HTTP server, select, accept, and read

Start the server on terminal A:

```text
piconix# httpd
```

On terminal B:

```text
piconix# trace on
piconix# trace follow
```

Access the Pico W HTTP server from a PC browser. Look for `select`, `accept`, `read`, and `write`. Relate the count of ready descriptors returned by `select` to subsequent `accept` and `read` operations. Because `httpd` normally continues running, do not use `trace run httpd` in this exercise.

### Exercise 9: blocking accept versus I/O multiplexing

On terminal A:

```text
piconix# tcp_echod 7000
```

Capture events using `trace on` and `trace follow` on another terminal, and connect with an external TCP client on port 7000 to exchange data. Compare events during idle waiting and after connections or data arrive. Next, compare with communication through `nc` to study blocking on one socket versus using `select` for multiple descriptors. Confirm the precise wait states in the user-space and kernel sources; the trace alone may not identify every internal state transition.

### Exercise 10: PID filtering and SysTick

```text
piconix# ps
piconix# trace on
piconix# trace follow
```

Find periodic `TIMER TICKx1K` events and `SWITCH` records. `TICKx1K` reports every 1,000 ticks of the 1 ms SysTick; it does not report every interrupt.

To focus on one program, obtain its PID using `ps`, then use `trace follow PID` or `trace show PID`. A PID argument is a **display filter**, not a capture filter, and does not reduce ring-buffer usage. `SWITCH` records are filtered by destination PID. To inspect PID-less asynchronous function events, use `trace on -a` with unfiltered `trace follow`.

### Lab caveats

- A build with `PICONIX_KERNEL_FUNCTION_TRACE=OFF` produces no `KERNEL` function rows.
- `-a` does **not** enable every kernel function. It admits non-SVC contexts only for selected and instrumented functions.
- The ring buffer holds 512 events and overwrites old entries. Start with short captures.
- `Ret` for function events is not the internal function's return value; distinguish it from syscall return values.
- Tracing adds runtime overhead, so displayed timestamps are not a precise performance benchmark.

## `/dev/trace` binary format

Records are **32 bytes each**, as defined by `trace_event_t` in `include/piconix_trace.h`. The kernel holds **16 KiB (512 events)** in a ring buffer; new events overwrite the oldest when full.

- `tick`: milliseconds since boot
- `kind`: low six bits are event type; high two bits encode TTY (0=console, 1=ttyS1, 2=ttyN0)
- `pid` / `program_id`: registered program ID (the event kind high bits encode TTY)
- `file_id`: internal device/file ID, or -1 when absent/unknown
- `value` / `detail`: syscall ID, arguments, return value, or destination task depending on event
- `data[16]`: first 16 bytes of a successful `open()` path or successful `read()`/`write()` transfer; not necessarily NUL-terminated

The viewer associates successful open events with PID/FD paths. If that history has been overwritten, it falls back to file IDs. Long paths and I/O are truncated, while binary bytes are escaped for display.

## Timing and limitations

SysTick runs every 1ms and the usual time slice is 10ms. A sleeping task can trigger rescheduling with 1ms granularity. Tracing has runtime overhead. A blocking `read()` EXIT is emitted when input arrives and the call completes, not when it first blocks. PID filtering does not reduce ring buffer consumption.

### Device names and IDs

Fixed device IDs and paths are shared between the kernel VFS and `trace` through `include/device_names.h`. `trace` prints a canonical path when available, falling back to `id=N` for unknown or dynamic devices. The TTY column shows `console`, `ttyS1`, or `ttyN0`. `ls -l` shows IDs for device nodes, program entries, and RAM-file slots; use `/proc/task/<pid>` or `lsof` to inspect which descriptors are open. Existing binary trace logs from older firmware should not be mixed with this format because TTY metadata moved into the event-kind byte.

---

**Related links:** [trace guide](TRACE.md) | [Guide index](README.md) | [日本語](../../ja/guides/TRACE.md) | [Internals](../reference/INTERNAL.md)
