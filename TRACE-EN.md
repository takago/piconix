# PicoNIX Kernel Trace

Use `trace on` to clear and start recording, `trace off` to stop and `trace show` to stop and display saved events.

## Binary ABI

`/dev/trace` exports fixed 32-byte events defined in `ram_apps/trace.h`. The 16-KiB ring holds 512 records, overwriting the oldest as needed.

- `tick`: elapsed milliseconds since boot.
- `kind`: SWITCH, SVC entry/exit, or TICKx1K.
- `pid`, `program_id`: task and registered program identifiers.
- `file_id`: internal device/file identifier, or -1 when absent or unknown.
- `value`, `detail`: syscall number, argument, result, or switch target depending on event kind.
- `data[16]`: first 16 bytes of a successful open path or successful read/write transfer, with no required NUL terminator.

## Timer

SysTick fires every 1 ms. Every thousandth tick is recorded as `TICKx1K` (1000 ms). Normal time slicing remains 10 ms, with earlier rescheduling when a sleeping task wakes. Actual task switches and SVC events are not sampled. `msleep` still takes milliseconds, with a 1-ms wakeup granularity.

## Descriptor names

The user-space trace decoder associates successful open paths with PID/FD pairs; subsequent read/write/close entries reuse this history. If the open event was overwritten, only the file ID is shown. Standard descriptors 0-2 may be inherited without open events.

## Limitations

Captured paths and I/O previews are truncated to 16 bytes. Binary data is escaped for display. Tracing introduces overhead; `trace show` first disables tracing to prevent recursive capture.

Time is displayed in seconds (for example, `12.460`). `TICKx1K` records every thousandth 1-ms SysTick. A blocking terminal `read()` does not log an EXIT at the initial wait; it logs EXIT and received bytes when the input is delivered and the task becomes ready.


## 80-column output and continuous follow

`trace show` stops capture and prints the retained history. `trace follow`
continuously displays new events without stopping capture. Start capture with
`trace on` first. `trace follow` runs in the foreground; issue `trace off`
from another terminal to stop capture and let the viewer drain and exit, or use
Ctrl+C according to the foreground-task terminal behavior.

Time is shown as seconds (`000.00`). The TTY column is `console`, `uart1`,
or `-`. ENTER/EXIT use aligned syscall, target, return, and data columns.
The `trace` program's own syscalls and context switches to/from that program
are excluded. Other SWITCH and sampled TICKx1K events remain enabled.

Long output is truncated to fit the terminal; each binary event still stores
up to 16 data bytes. The upper bit of program_id encodes UART1 TTY, preserving
the 32-byte event ABI. If live viewing falls behind the fixed ring, the viewer
reports the number of overwritten events.
