# PicoNIX Kernel Tracing

*Usage guide*

PicoNIX records task switches, system-call entry/exit, and periodic timer events through `/dev/trace`. Use the `trace` command to inspect these events.

## Start with one command

```sh
trace run hello
```

This launches `hello`, waits for it to finish, and displays events associated with that process. It is the easiest starting point. Read the columns as follows:

| Column | Meaning |
|---|---|
| `Time` | Time since boot when the event was recorded |
| `PID` / `Command` | Associated task and command |
| `TTY` | Terminal used |
| `Type` | Event category such as `SYSCALL`, `KERNEL`, `SWITCH`, or `TIMER` |
| `Function` | Operation name; a trailing `/` marks an exit |
| `Arg` | Captured arguments, FD, or path (not necessarily every argument) |
| `Ret` | Recorded syscall result |
| `Info` | Supplemental information, such as a preview of I/O data |

For example, `open` marks entry and `open/` marks exit. The entry `Arg` can show the requested path, up to 16 bytes; check the exit for the result. Longer paths are truncated. `--` means no value was captured for that field. Timestamps and event counts depend on the run.

Next, `trace run ls` provides a richer file-operation example.

## Commands

```text
trace on -s      # record syscall entry/exit only
trace on         # clear the buffer and start recording (SVC functions)
trace on -a      # record selected functions outside SVC too
trace off        # stop recording
trace show       # stop recording and display all retained events
trace show 3     # stop recording and show events for PID 3
trace follow     # display events occurring after invocation
trace follow 3   # display new events for PID 3 after invocation
trace run -s hello # trace syscalls only
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

### Polling/readiness checks and function instrumentation

`-finstrument-functions` inserts entry/exit hooks into functions in selected source files. Under `trace on -a`, frequent readiness checks can fill the trace ring even without network traffic.

The `NO_TRACE` (`no_instrument_function`) attribute therefore suppresses instrumentation of TCP/TTY readiness and status probes and scheduler wake-check loops. Those probes are also excluded from `TRACE_MATCH`. The actual `tty_input_char()` input path, socket data transfers, `task_unblock()`, `SYSCALL`, `TIMER`, and `SWITCH` remain observable.


In the eight instrumented kernel C sources, **only the 51 functions listed in `TRACE_MATCH`** retain compiler entry/exit instrumentation. Every other function definition is marked `NO_TRACE`. This avoids invoking instrumentation hooks for functions that would never appear in the trace. Polling itself still runs, but the trace overhead is reduced. When adding a function, check whether it belongs in `TRACE_MATCH` or needs `NO_TRACE`, and run `test_kernel_trace_targets.py` to detect omissions.

`NO_TRACE` does not affect called functions and does not stop polling itself. Validate the remaining UART1 interrupt/input instrumentation overhead with continuous input on real hardware.

### Selected functions and the symbol table

When instrumentation is enabled, the compiler adds `-finstrument-functions` hooks to `kernel/kernel.c`, `kernel/adc.c`, `kernel/i2c.c`, `kernel/pwm.c`, `kernel/gpio.c`, `kernel/fb.c`, `kernel/net_socket.c` and `kernel/net_capture.c`. The `TRACE_MATCH` list in `kernel/kernel.c` filters which function calls are actually recorded. A function defined in another C file can be referenced through its declaration in an internal header. Instrumented functions and recorded functions are not necessarily the same set.

The kernel symbol table shown by `mem -s kernel` is instead generated from the linked ELF. It provides symbol names and addresses for inspection; it does not select trace targets. Run `tools/test_scripts/test_kernel_trace_targets.py` to check the selected list for duplicates, source definitions and instrumentation coverage.

The target list now also includes `os_svc_dispatch()` (SVC dispatch) and `task_unblock()` (resuming blocked tasks). The former intentionally overlaps the existing `SYSCALL` events to show the function boundary. The latter can run outside SVC, so observing it may require `trace on -a`. `tty_input_char()` is now included experimentally. It processes input characters from USB CDC and Telnet and can run in the UART1 receive interrupt once per character. Use `trace on -a` to observe non-SVC calls. Check UART1 continuous reception for lost bytes on hardware, as interrupt overhead and ring-buffer overwrites may increase. PID-less interrupt events are not shown by the PID-filtered `trace run -a` output.

Coverage includes selected VFS, file-descriptor, RAM-file, TTY, ADC, I2C, PWM, GPIO,
framebuffer and TCP/UDP functions. ICMP device calls `net_echo_write`, `net_echo_read`,
and `net_echo_close` are included, but `net_capture_poll` and `echo_received` are not.
The existing record ABI is 32 bytes, with at most 16 bytes for function names, and the
shared 16 KiB ring stores 512 events. Function entries/exits normally appear as `name` / `name/`. **An immediately adjacent entry/exit for the same function, PID, and recorded millisecond** is condensed to a single `(name)` row at display time. A nested call, intervening event, or differing timestamp prevents condensation. The underlying records and buffer usage are unchanged. `trace follow` briefly holds at most one entry record for matching, then prints it separately when no further records are available.
Display indentation is approximate, especially across interrupts or overwritten events.
Compiler optimizations may inline calls. All-context recording increases runtime overhead.
The ring buffer is protected against interrupt-time re-entry, but real-device timing and
capacity must still be validated. The old `trace kernel` command has been removed.

#
## Event types and display

| Event | Meaning |
|---|---|
| `SWITCH` | Context switch (destination PID is displayed) |
| `read` / `read/` etc. | System-call entry and exit, using the syscall name |
| `TICKx1K` | One record for every 1,000 ticks of the 1ms SysTick |
| `vfs_rw` / `vfs_rw/` etc. | Entry/exit of selected kernel functions (experimental) |

The display contains `Time | PID | Command | TTY | Type | Function | Arg | Ret | Info`. The `Type` column distinguishes `SYSCALL`, `KERNEL`, `SWITCH`, `TIMER`, and `OTHER`. With `os_svc_dispatch()` instrumented, the syscall row is nested inside it, and internal kernel calls are indented one level further. Without the dispatcher event, syscall rows start at depth 0. `Arg` carries the previously displayed target or recorded argument, not necessarily every function argument. `Ret` is the recorded syscall return value; internal kernel-function return values are not captured. `Info` contains auxiliary details such as I/O previews or task switches. Missing or inapplicable fields show `--`. The `exit` record is an OS termination event, not a conventional function return. The trace command's own system calls and related context switches are excluded from capture.

## Try it: observe system calls and task switches

```text
trace run hello
trace run ls
```

After each program exits, its trace is displayed. Compare `function` and `function/` records to see which system calls it made. `SWITCH` marks a context switch; filtering uses the destination PID.

For a longer capture, start with `trace on`, run another command, then inspect the result using `trace show` (which stops capture). On long runs, older events may be overwritten in the ring buffer.



## Kernel function and syscall nesting; switch emphasis

When function instrumentation includes `os_svc_dispatch()`, `SYSCALL` entry/exit rows appear one level beneath that function. For example, `os_svc_dispatch` → `write` → `vfs_rw` → `write/` → `os_svc_dispatch/` shows the nesting. Without function instrumentation, syscalls remain at the top level. Indentation is reconstructed from trace events and may not represent an exact call stack after dropped events or interrupts.

`os_svc_dispatch/` means the C dispatcher returned; it does not prove completion of exception return. No extra `SVC_ENTER` or `SVC_LEAVE` boundary event is recorded.

When the displayed PID changes from the preceding displayed row, an ASCII hyphen (`-`) separator is printed immediately before the new row. No separator appears before the first event. Events removed by the PID filter do not affect this comparison; compacted function entry/exit pairs count as one displayed row. A `SWITCH` row displays the destination PID, so a separator is a change in the displayed PID, not necessarily the exact instant of a context switch. No ANSI control sequences are needed.

## Internals: `/dev/trace` binary format

Records are **32 bytes each**, as defined by `trace_event_t` in `include/piconix_trace.h`. The kernel holds **16 KiB (512 events)** in a ring buffer; new events overwrite the oldest when full.

- `tick`: milliseconds since boot
- `kind`: low six bits are event type; high two bits encode TTY (0=console, 1=ttyS1, 2=ttyN0)
- `pid` / `program_id`: registered program ID (the event kind high bits encode TTY)
- `file_id`: internal device/file ID, or -1 when absent/unknown
- `value` / `detail`: syscall ID, arguments, return value, or destination task depending on event
- `data[16]`: first 16 bytes of an `open()` path on entry (including failed calls) and on successful exit, or a successful `read()`/`write()` transfer; not necessarily NUL-terminated

The `Arg` column for `open` on ENTER shows the requested path (up to 16 bytes). On successful `open/` EXIT, the FD is associated with that path. On failed EXIT, the viewer reuses the path recorded at ENTER. If the path cannot be read or is empty, ENTER displays `path`.

The viewer associates successful open events with PID/FD paths. If that history has been overwritten, it falls back to file IDs. Long paths and I/O are truncated, while binary bytes are escaped for display.

## Timing and limitations

SysTick runs every 1ms and the usual time slice is 10ms. A sleeping task can trigger rescheduling with 1ms granularity. Tracing has runtime overhead. A blocking `read()` EXIT is emitted when input arrives and the call completes, not when it first blocks. PID filtering does not reduce ring buffer consumption.

### Device names and IDs

Fixed device IDs and paths are shared between the kernel VFS and `trace` through `include/device_names.h`. `trace` prints a canonical path when available, falling back to `id=N` for unknown or dynamic devices. The TTY column shows `console`, `ttyS1`, or `ttyN0`. `ls -l` shows IDs for device nodes, program entries, and RAM-file slots; use `/proc/task/<pid>` or `lsof` to inspect which descriptors are open. Existing binary trace logs from older firmware should not be mixed with this format because TTY metadata moved into the event-kind byte.

---

**Related links:** [trace guide](TRACE.md) | [Guide index](README.md) | [日本語](../../../ja/reference/commands/TRACE.md) | [Internals](../INTERNAL.md)
