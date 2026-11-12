# CDC Virtual Console

PicoNIX multiplexes **two independent logical TTYs** over one physical USB CDC port. UART1, Telnet and the MCP PTY keep their existing numbers and I/O paths.

| Console | TTY number | Device | Shell |
|---|---:|---|---|
| Console 0 | 0 | `/dev/ttyV0` | Starts at boot |
| Console 1 | 4 | `/dev/ttyV1` | Starts on first activation |

`/dev/console` refers to the **currently selected CDC virtual console**. At `open()`, it resolves to TTY 0 or TTY 4; later Ctrl+T changes do not redirect an already-open descriptor. Shell standard I/O and `/dev/tty` remain attached to the task’s own terminal. `/dev/ttyV0` and `/dev/ttyV1` are stable device paths.

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

## Vertically stacked dual display

Press **Ctrl+G** (`0x07`) on USB CDC to toggle between the original single-console view and the vertically stacked dual display. The two independent 80×24 screen buffers are drawn one above the other. Both `[/dev/ttyV0]` and `[/dev/ttyV1]` headers are full-width (80-column) reverse-video bars; the selected header also shows a bold `ACTIVE` label. The bottom help line is another full-width reverse-video bar and reads `C-Left/Right: V0/V1/Dual  C-Up/Down: Focus (Dual)  C-T/G: Legacy`. **Ctrl+T** selects the input console in either mode. Headers and help text are never stored in the application screen buffers.

Use an ANSI-capable host terminal of **at least 80 columns × 51 rows**: `/dev/ttyV0` header on row 1, screen on rows 2–25, `/dev/ttyV1` header on row 26, screen on rows 27–50, and help on row 51. In dual mode, application output updates its screen buffer; USB redraw emits positioned screen content instead of forwarding raw escape sequences, isolating scrolling, clearing, and cursor movement between the two views. The existing bounded USB transmit queue is used. The cursor stays hidden throughout the incremental redraw and is restored at the selected TTY position only once the panels are complete, reducing cursor flicker.

Single mode still forwards long lines to the host terminal, but **dual mode displays only the captured 80×24 area of each console**. Complex ANSI operations remain limited by the existing minimal screen parser. Dual mode generates more USB redraw traffic and requires the stated host terminal height. Ctrl+G is reserved by USB CDC and is not passed to applications.


### Ctrl+arrow shortcuts (alternative)

When the USB CDC host terminal sends xterm-style sequences (`ESC [ 1 ; 5 A/B/C/D`), **Ctrl+Right** cycles `ttyV0 single → ttyV1 single → stacked dual → ttyV0 single`; **Ctrl+Left** cycles in reverse. In dual mode only, **Ctrl+Up** selects `/dev/ttyV0` as the input TTY and **Ctrl+Down** selects `/dev/ttyV1`. In single mode, Ctrl+Up/Down do not change the view. Leaving dual mode selects the TTY of the resulting single view. Ctrl+T and Ctrl+G remain supported.

Plain arrow keys and Escape are forwarded to applications. Incomplete sequences are forwarded to their original TTY after a brief timeout. Some terminal applications or VS Code shortcuts may intercept Ctrl+arrows; use Ctrl+T / Ctrl+G if they are not sent to PicoNIX.
