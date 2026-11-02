# Terminal Settings and Interactive Input

[日本語](../ja/TTY.md) | [Overview](README.md) | [UART1 and terminals](UART.md) | [C programming](PROGRAMMING.md)

PicoNIX keeps **terminal input modes** separate from **file-descriptor (FD) blocking flags**. It does not implement the complete POSIX `termios` interface.

## Terminal modes: `stty` and `ioctl()`

```text
stty
stty -icanon -echo
stty icanon echo
stty raw
stty cooked
```

- `stty` shows the `icanon` (canonical line input) and `echo` flags of standard input.
- `icanon` / `-icanon` selects line-based / character-based input.
- `echo` / `-echo` enables / disables input echo.
- `raw` is shorthand for `-icanon -echo`; `cooked` is shorthand for `icanon echo`. These are not the full POSIX raw/cooked configurations.

C applications can use `ioctl()` with `OS_TTY_GET_CANON`, `OS_TTY_GET_ECHO`, `OS_TTY_SET_CANON`, and `OS_TTY_SET_ECHO`. `stty` calls `OS_TTY_PERSIST` to retain settings after it exits. For foreground applications, the kernel restores saved TTY settings when the task exits. In pipelines, terminal input ownership passes to a surviving stage when a stage exits early, and saved settings are restored after the last stage exits; `edit`, `mem`, `gpio`, `mb`, `pong`, and `fbed` therefore do not restore these settings on normal exit. Applications retain startup-failure rollback. `VMIN` and `VTIME` are not supported.

## Nonblocking I/O: `fcntl()`

Use `fcntl(fd, F_GETFL, 0)` to read FD flags, and `fcntl(fd, F_SETFL, flags)` to change `O_NONBLOCK`. This is independent of canonical and echo modes. For immediate character input, select noncanonical mode and an appropriate blocking policy.

In the current implementation, `F_SETFL` can change `O_NONBLOCK` on the USB CDC console, UART1, the Wi-Fi TTY (when enabled), and TCP/UDP socket descriptors. It is not supported for ordinary files, pipes, or other devices. At `open()` time, `O_NONBLOCK` is accepted only for `/dev/console` and `/dev/ttyS1`; for `/dev/ttyN0`, set it afterward with `fcntl()`.

When no input is available, a nonblocking TTY `read()` returns **0** (not a negative `EAGAIN`-style error). Do not always interpret that 0 as permanent EOF. Pipes and sockets have different I/O semantics; see [Pipes](PIPE.md) and [Socket APIs](NETWORK-PROGRAMMING.md).

## Restoration and screen control

`stty` and `reset` use `OS_TTY_PERSIST` to retain deliberate configuration changes. When a foreground task exits or is interrupted, the kernel restores its saved terminal settings. Interactive programs such as `mb` combine noncanonical input with nonblocking descriptors. `reset` does not change `O_NONBLOCK`.

Cursor visibility, alternate screens, and text attributes are separate from TTY settings. For application screen cleanup, practical `reset` use, and interruption caveats, see [Terminal usage and screen control](TERMINAL.md).

## TTY output newline conversion (ONLCR)

By default, the USB CDC console and UART1 terminal convert LF (`\n`) to CRLF (`\r\n`). An LF immediately following CR is passed through, so existing CRLF sequences are not doubled. Files and non-TTY devices are unaffected.

```text
stty             # Show onlcr or -onlcr
stty -onlcr      # Disable output conversion
stty onlcr       # Enable output conversion
```

C applications can use `ioctl(fd, OS_TTY_GET_ONLCR, 0)` and `ioctl(fd, OS_TTY_SET_ONLCR, 0 or 1)`. The setting is per terminal and independent of canonical input and echo. The `raw` convenience command does not change output conversion.

## TAB input

TAB (`0x09`) is accepted by shell line editing and canonical input and is preserved as a TAB byte in files. It is rendered using four-column tab stops, with spaces up to the next stop. Shell cursor movement, deletion, and history redraw account for the displayed width.

## Ctrl+L in the shell

During shell line editing, Ctrl+L clears the terminal screen using ANSI sequences and redraws the prompt and current input. The line, selected history entry, and cursor position are preserved. Unlike `reset`, this does not change terminal settings.


## TTY versus pipes and sockets

`stty` and `OS_TTY_*` `ioctl()` operations configure **terminals**. Canonical input and echo do not apply to pipes, ordinary files, or sockets. `O_NONBLOCK` via `fcntl()` controls descriptor I/O behavior; for pipes and sockets, consult the relevant API behavior when no data is available.

- `icanon` enables terminal line input; disabling it allows character input. **Pipes do not perform terminal line editing.**
- `echo` is local terminal input echo, not a property of pipe or network data.
- `onlcr` converts LF to CRLF **only for TTY output**, not pipe, file, or socket byte streams.
- POSIX `termios` `VMIN` / `VTIME` are **not implemented**. Use `fcntl()` and handle `read()` results as appropriate.
- USB CDC, UART1, and the Wi-Fi terminal are separate TTYs when enabled in the build.


**Implementation references:** `tty_canonical`, `tty_echo`, `tty_onlcr`, `tty_saved_*`, `foreground_group`, and `OS_TTY_*` handling in `kernel/kernel.c`; line editing and pipelines in `userland/shell.c`.
