# Source-first reading exercises

For students already familiar with UNIX/Linux, basic OS concepts and C. Trace **observable command → syscall → kernel state change → return value**. PIDs and timestamps are not fixed.

| Exercise | Observe on PicoNIX | Read these sources | Verify |
|---|---|---|---|
| Boot and shell | `ps`, `ls /bin` | `kernel/main.c`, `kernel/kernel.c`, `userland/apps/shell.c` | Image registration, task creation, `/bin` registration |
| FDs and VFS | `echo hello > /tmp/a`, `cat /tmp/a` | `userland/lib/syscalls.c`, `include/piconix_syscall.h`, `kernel/context.S`, `kernel/kernel.c` | SVC numbers and per-task descriptors |
| Pipes | `echo hello | cat -n` | `userland/apps/shell.c`, `kernel/kernel.c` | `pipe/dup2/close` and EOF |
| Tracing | `trace run -s hello` | `userland/apps/trace.c`, `kernel/kernel.c`, `include/piconix_trace.h` | ENTER vs EXIT; `select()` is syscall 40 |
| Devices | `cat /dev/fbchar`, `cat /proc/fb` | `include/device_names.h`, `kernel/kernel.c`, `kernel/fb.c` | Path dispatch and device operations; check wiring first |
| RAM and PIC | `cat /proc/memory`, `mem` | `userland/lib/slot.ld`, `kernel/main.c`, `kernel/kernel.c`, `tools/test_ram_pipeline.py` | XIP, 24 KiB slots, 4 KiB stack reserve, PIC testing |
| Networking (Pico W) | `ip`, `ping 127.0.0.1` | `kernel/net.c`, `kernel/net_socket.c`, `userland/apps/ping.c` | lwIP boundary and socket behavior |
| MCP (Pico W) | `mcpd &`, `host/mcp_client.py` | `userland/apps/mcpd.c`, `host/mcp_client.py` | TCP, HTTP, JSON-RPC and MCP separation |

## Method

1. Check argument syntax and cautions in the [usage guides](../guides/README.md).
2. Find API and numbers in the [syscall reference](../reference/SYSCALLS.md), then confirm in `include/piconix_syscall.h`.
3. Locate functions using the generated [function index](../../FUNCTIONS.md); regenerate it after code changes.
4. Follow return values, errors and blocking paths. Compare with [Linux differences](../reference/DIFFERENCES.md).
5. Make changes on a separate branch; run `python3 tools/run_tests.py`, SDK build and hardware verification as appropriate.

[Exercises](README.md) | [Kernel reading guide](../development/kernel/KERNEL-READING-GUIDE.md) | [日本語](../../ja/exercises/READING-PATHS.md)
