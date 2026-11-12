# CDC Virtual Console

PicoNIX multiplexes **two independent logical TTYs** over one physical USB CDC port. UART1, Telnet and the MCP PTY keep their existing numbers and I/O paths.

| Console | TTY number | Device | Shell |
|---|---:|---|---|
| Console 0 | 0 | `/dev/console` | Starts at boot |
| Console 1 | 4 | `/dev/ttyV1` | Starts on first activation |

Press **Ctrl+T** (byte `0x14`) in the USB CDC terminal to toggle between consoles. Programs on the hidden console continue executing. Ctrl+T is reserved for console switching and is not delivered to applications. If no user task slot is free for the second shell, a bell sounds and the current console remains selected.

## Example

1. Boot PicoNIX over USB CDC.
2. Run `top` on Console 0.
3. Press Ctrl+T to switch to Console 1, and run `tty` to see `/dev/ttyV1`.
4. Run a game or Python on Console 1, then press Ctrl+T to return to `top`.

Canonical mode, echo, ONLCR, Ctrl+C, foreground ownership, input queues and 80×24 display state are maintained separately for each TTY. Restoring a display uses small chunks of the existing USB transmit queue, so a full redraw may take a moment.


## Fixed-width screen capture without automatic wrapping

Each captured screen stays 80×24. Once column 80 is written, subsequent printable characters on that line are **discarded by the screen recorder instead of wrapped**; a newline or cursor movement resumes capture at its new position. This is display-layer behavior, not TTY line discipline. Bytes delivered over physical USB CDC are not truncated: a wider host terminal can still show long `trace` lines. After switching away and back, however, only the recorded 80×24 area can be restored. The PTY `/dev/pts/0.screen` uses the same screen parser.

A shared TTY device-to-line-discipline mapping is used by I/O and ioctl paths, while existing canonical/raw input, echo, terminal modes and foreground-task management remain common to CDC, UART1, Telnet and PTY. Screen buffers remain separate from the TTY layer.

## Limitations

- Screen reconstruction uses the same minimal ANSI interpreter as the PTY screen. Basic ASCII text, scrolling, cursor motion, clearing and reverse video are restored.
- **It is not a full terminal emulator.** Colors, scroll regions and more complex ANSI sequences may not be reconstructed correctly after switching away and back. Complete restoration of the original normal-screen history after alternate-screen exit is not implemented.
- Ctrl+T is intercepted on USB CDC only; UART1, Telnet and `/dev/pts/0` behavior remains unchanged. Your host terminal may need its Ctrl+T shortcut unbound.
- Console 1 requires one of the existing six user task slots for its shell. No new management task, syscall or user RAM slot is added.
- Trace events encode the TTY number in only two bits. TTY 4 therefore shares the physical USB CDC trace tag with TTY 0; PIDs still identify the originating task.

See [TTY](../commands/TTY.md), [PTY](PTY.md), and [terminal usage](../commands/TERMINAL.md).
