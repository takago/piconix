# PicoNIX Kernel Tracing

[日本語](../ja/TRACE.md) | [Overview](README.md) | [Internals](INTERNAL.md)

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
```

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

The existing 32KiB (1,024-event) ring buffer is still used. Long-running programs may overwrite their earliest records, including startup events. Only the directly spawned process is filtered; descendants are not automatically followed. Interactive programs must exit before the trace report is printed.

## Event types and display

| Event | Meaning |
|---|---|
| `SWITCH` | Context switch |
| `ENTER` / `EXIT` | System-call entry and exit |
| `TICKx1K` | One record for every 1,000 ticks of the 1ms SysTick |

The display contains time, PID, command, TTY (`console` / `uart1` / `-`), event, call, target, return value, and data columns. The trace command's own system calls and related context switches are excluded from capture.

## `/dev/trace` binary format

Records are **32 bytes each**, as defined by `trace_event_t` in `userland/trace.h`. The kernel holds **32KiB (1,024 events)** in a ring buffer; new events overwrite the oldest when full.

- `tick`: milliseconds since boot
- `kind`: event type
- `pid` / `program_id`: task and registered program IDs (the latter also encodes the TTY)
- `file_id`: internal device/file ID, or -1 when absent/unknown
- `value` / `detail`: syscall ID, arguments, return value, or destination task depending on event
- `data[16]`: first 16 bytes of a successful `open()` path or successful `read()`/`write()` transfer; not necessarily NUL-terminated

The viewer associates successful open events with PID/FD paths. If that history has been overwritten, it falls back to file IDs. Long paths and I/O are truncated, while binary bytes are escaped for display.

## Timing and limitations

SysTick runs every 1ms and the usual time slice is 10ms. A sleeping task can trigger rescheduling with 1ms granularity. Tracing has runtime overhead. A blocking `read()` EXIT is emitted when input arrives and the call completes, not when it first blocks. PID filtering does not reduce ring buffer consumption.
