# `top` — Live CPU and process monitor

`top [seconds]` reads `/proc/cpu`, `/proc/tasks`, `/proc/uptime`, and `/proc/context_switches`, refreshing the terminal approximately every second. The optional interval is 1–60 seconds. Press `q` to quit.

```sh
top
top 2
```

The display includes uptime, system-wide CPU usage (Idle, Busy, Console, Task, Net), task count, cumulative context switches, and PID, NAME, STATE, TTY, SLOT, MEMORY, SP, and PPID from `/proc/tasks`. Output is limited to 79 columns and only changed rows are redrawn; an ANSI terminal of at least 80 columns is recommended.

**Limitations:** CPU usage is an **approximation** from the newest completed one-second sample, not per-process accounting. No percentage is available until the first sample is complete. `top` occupies one of the six task slots. Terminal modes are restored on exit.

For MCP, run interactively in the persistent PTY, not via the noninteractive `execute_command` tool.
