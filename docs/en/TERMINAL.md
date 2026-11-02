# Terminal Usage and Screen Control

[日本語](../ja/TERMINAL.md) | [TTY settings](TTY.md) | [UART and terminals](UART.md) | [Historical debugging notes](../history/en/TERMINAL-DEBUG.md)

In PicoNIX, **the kernel manages TTY input/output behavior**, while **applications control cursor position, full-screen display, and other screen features by emitting ANSI/DEC escape sequences**. These layers are independent: restoring TTY settings does not necessarily restore the screen.

## 1. Terminals and I/O

Shells can use the USB CDC `/dev/console`, UART1 `/dev/ttyS1`, or `/dev/ttyN0` in supported Pico W builds. See [UART and terminals](UART.md) and [Networking](NETWORK.md) for connection details and `getty`.

TTY processing includes canonical line input, local echo, and ONLCR output newline conversion. Pipes, ordinary files, and sockets are not TTYs and do not receive this processing.

## 2. TTY settings versus descriptor blocking

Canonical input (`icanon`), input echo (`echo`), and output newline conversion (`onlcr`) are per-terminal settings configured by `stty` or `ioctl()`. In contrast, `fcntl()` with `O_NONBLOCK` controls descriptor I/O waiting behavior. These are independent. `raw` only means `-icanon -echo`; it is not full POSIX raw mode.

For exact commands, `OS_TTY_*`, `OS_TTY_PERSIST`, restoration rules, and limitations, see [TTY settings](TTY.md). This guide concentrates on application-managed screen control.

## 3. Newlines and screen display

| Character | Typical effect |
| --- | --- |
| `\r` (CR) | Return to the start of the line |
| `\n` (LF) | Move to the next line |
| `\r\n` | Move to the beginning of the next line |

When ONLCR is enabled, TTY output converts LF to CRLF without doubling existing CRLF. This is **kernel-side TTY output processing**, not arbitrary cursor positioning.

Applications output escape sequences, which are interpreted by the terminal emulator on the host computer.

| Sequence | Typical purpose |
| --- | --- |
| `ESC[2J` | Clear the display |
| `ESC[H` | Move cursor home |
| `ESC[?25l` / `ESC[?25h` | Hide / show cursor |
| `ESC[0m` | Reset text attributes |
| `ESC[?1049h` / `ESC[?1049l` | Enter / leave alternate screen |
| `ESC[r` | Reset scrolling region |

In C strings, write `ESC` as `\033`. Emulator support and scrollback behavior vary.

## 4. Full-screen application conventions

Full-screen programs such as `edit`, `mem`, `gpio`, and `mb` change the display on entry and restore it on exit. Conceptually:

```c
printf("\033[?1049h\033[?25l"); /* Alternate screen; hide cursor */
/* Interactive work */
printf("\033[?25h\033[?1049l"); /* Restore */
```

**The kernel restores saved canonical/echo/ONLCR settings when foreground tasks terminate**, except for intentional persistent changes made by `stty` or `reset`. **ANSI/DEC display state is not part of kernel TTY settings**. Applications should clean up on normal exit; the current shell also attempts alternate-screen restoration after certain named full-screen commands finish or are interrupted. Review this handling when adding new full-screen programs.

Do not emit `ESC[?1049l` at every ordinary shell prompt: some emulators restore a saved cursor position and disrupt output.

## 5. Ctrl+C, pipes, and terminals

When Ctrl+C interrupts a foreground task, normal application cleanup may be skipped. Distinguish kernel TTY restoration from screen restoration. In pipelines, `dup2()` may redirect standard I/O to pipes, so `stdout` is not necessarily a TTY. Programs writing pipeline data should avoid injecting screen-control sequences. See [Pipes](PIPE.md).

## 6. Recovering with `reset`

```sh
reset
```

`reset` enables canonical input and echo and attempts to leave alternate screens, reset text attributes and scrolling regions, show the cursor, and clear the display. It does not modify `O_NONBLOCK`. It cannot help if the user cannot enter commands at all.

Earlier gtkterm/VS Code display issues, troubleshooting procedures, and incremental-build notes have moved to the [development history](../history/en/TERMINAL-DEBUG.md).

## 7. Incremental screen updates with `watch`

`watch [seconds] command [args...]` captures the child's stdout and stderr through a pipe, then compares the completed output to the previous snapshot. Only changed lines are rewritten using ANSI cursor positioning. The screen is cleared once on entry; shorter lines are cleared to end-of-line.

```sh
watch 1 ps
watch 1 cat /proc/net/memory
```

Output is limited to a fixed display region for terminals with at least 24 rows: 20 content rows, up to 78 bytes per row, below the header. Additional output is always drained and marked with `[watch: output truncated]`. Comparison is line-oriented, terminal control sequences in captured output are suppressed, and interactive/full-screen programs are unsupported. Commands that do not terminate prevent the next refresh, as before.

To avoid deadlocking on the kernel's 256-byte pipe, `watch` drains the pipe to EOF **before** calling `waitpid()`. Both screen snapshots live in the user process; the kernel does not store display history.

`mem` and `watch` temporarily disable terminal autowrap during repainting, clipping text at the terminal edge instead of wrapping to the next row, and restore autowrap afterward.
