# PicoNIX Kernel Tracing

*Usage guide*

PicoNIX records task switches, system-call entry/exit, and periodic timer events through `/dev/trace`. Use the `trace` command to inspect these events.

## Commands

```text
trace on         # clear the buffer and start recording
trace off        # stop recording
trace show       # stop recording and display all retained events
trace show 3     # stop recording and show events for PID 3
trace follow     # display events occurring after invocation
trace follow 3   # display new events for PID 3 after invocation
trace run hello  # start hello and show its trace after exit
trace run python test.py  # trace a Python script
trace kernel cat /tmp/test.txt  # also trace selected kernel functions (special build)
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

## Kernel function calls (experimental)

`trace kernel command [args...]` records selected internal kernel function entries and exits as `function` / `function/` in addition to the usual system calls. This is an **experimental feature, disabled by default at build time**. Ordinary syscall tracing with `trace run` remains available when it is OFF. To enable kernel function tracing, build firmware for Pico or Pico W with:

```bash
./tools/build_sdk.sh -DPICO_BOARD=pico -DPICONIX_KERNEL_FUNCTION_TRACE=ON
# Use -DPICO_BOARD=pico_w for Pico W
```


To disable instrumentation again, rebuild with `-DPICONIX_KERNEL_FUNCTION_TRACE=OFF`. CMake caches options in the build directory: if a previous build used `ON`, simply omitting the option may leave it enabled. **Pass `OFF` explicitly** or recreate the relevant board's build directory. See the [setup guide](../setup/SETUP.md) for build commands.

```text
piconix# echo hello > /tmp/test.txt
piconix# trace kernel cat /tmp/test.txt
```

The monitored functions cover VFS and FDs (`vfs_open`, `vfs_rw`, `vfs_fixed_path_id`, `vfs_directory_id`, `free_fd`, `fd_set`, `fd_release`, `fd_copy`, `in_task_range`), RAM files (`tmp_open`, `tmp_rw`, `tmp_unlink`, `tmp_init`, `tmp_name`, `tmp_lookup`), devices (`device_rw_tty`, `tty_rx_read`, `tty_rx_ready`, `tty_dequeue`, `device_rw_rand`, `rand32`, `device_rw_proc`, `proc_device`, `device_ioctl`, `device_ioctl_tty`), and `vfs_getdents` and `pipe_transfer`. Events are retained **only while handling a synchronous SVC exception**, not for arbitrary interrupts, scheduler activity, or every kernel function. Compiler optimizations and inlining may affect the results.

With `trace kernel omikuji`, the `/dev/rand` read may show `vfs_rw` → `device_rw_rand` → `rand32`; `trace kernel cat /tmp/test.txt` can show deeper path/FD allocation and RAM-file operations. Compiler optimizations can suppress some calls. Per-character TTY output helpers remain excluded to limit trace volume.

The existing 32-byte event ABI is preserved: `value` contains the function address and `data[16]` its name. `trace kernel` disables function tracing when its child exits; ordinary `trace run` is unchanged.

**Caution:** Enabling this option adds compiler instrumentation to **all functions in `kernel/kernel.c`** even though only the selected names are stored. This increases Flash size, stack usage, and runtime overhead. Keep the default `OFF` for normal releases. Function and syscall records share the existing 16 KiB ring buffer, so older events can be overwritten. Hardware verification on Pico and Pico W remains necessary.

`trace kernel` shows entry as `function` and return as `function/`, indenting nested kernel functions by two spaces per level. The display depth resets at syscall entry and exit, and is maintained separately per PID for `trace show` and `trace follow`. This is **display-only indentation**, not kernel stack unwinding. If trace records are overwritten or an entry/return event is missing, indentation may not reflect the exact call stack.


### Limit the displayed function depth

Use `trace kernel -d N command [args...]` with a depth from `0` to `8`. System calls are depth 0, directly nested kernel functions are depth 1, and omitting `-d` preserves the original, unlimited depth display.

```text
piconix# trace kernel -d 0 hello       # system calls only
piconix# trace kernel -d 1 hello       # directly called kernel functions
piconix# trace kernel -d 2 hello       # two kernel levels
piconix# trace kernel -d 8 omikuji     # up to eight levels
```

This is a **display filter only**: it does not reduce instrumentation or the number of events stored in the 16 KiB ring buffer. Hidden function events still update the depth counter, keeping indentation consistent. Place `-d` before the program name.

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
