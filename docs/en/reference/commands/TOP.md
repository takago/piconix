# `top` — Live CPU and process monitor

`top [seconds]` reads `/proc/tasks`, `/proc/uptime`, `/proc/ticks`, `/proc/context_switches`, and `/proc/interrupts`, refreshing the terminal approximately every second. The optional interval is 1–60 seconds. Press `q` to quit.

```sh
top
top 2
```

The display includes uptime, system-wide CPU usage (Idle, Busy, Console, Task, Net), task count, cumulative context switches, cumulative interrupt/exception counts (IRQ), and PID, NAME, STATE, TTY, CPU%, DISPATCH, and PPID from `/proc/tasks`. Output is limited to 79 columns and only changed rows are redrawn; an ANSI terminal of at least 80 columns is recommended.

**Limitations:** `top` computes the CPU summary and per-task percentages from one refresh interval using cumulative task ticks plus console/network time measured in idle context. Each task is sampled at 1 ms resolution, so percentages are approximate. The first refresh has no baseline and does not show percentages. `top` occupies one of six task slots. Terminal modes are restored on exit.

For MCP, run interactively in the persistent PTY, not via the noninteractive `execute_command` tool.

`DISPATCH` counts scheduler selections that grant a task CPU execution, including its initial run and re-selection of the same task. The count resets when a task slot is reused. This is not CPU time. `ps` still displays `SP` from `/proc/tasks`, which `top` omits to fit 80-column terminals.

`DISPATCH` does not count interrupts or SVC entries by themselves. Returning to the interrupted task without scheduler selection does not increment it. The system-wide `/proc/context_switches` retains its original meaning: switches between different tasks.

`[idle]` appears as the special PID 0 task in `ps`, `top`, and `/proc/tasks`. It consumes no user RAM slot. Its `DISPATCH` counts scheduler selections too. The `[idle]` CPU% subtracts console/network polling time from idle-context samples. Console and Net are not separate tasks, and appear only in the CPU summary. `/proc/tasks` includes internal cumulative `NET_US` and `CON_US` fields; only PID 0 carries those totals. The existing `/proc/cpu` retains its independent one-second history, which can represent a different interval.

`IRQ` shows cumulative SysTick, PendSV, SVC, and UART1 counts since boot. `SVC` is a software exception; PendSV counts do not necessarily match context-switch counts. The display reads existing `/proc/interrupts` counters without adding kernel counters.
