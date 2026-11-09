# Terminal Settings and Interactive Input

*Usage guide*

PicoNIX keeps **terminal input modes** separate from **file-descriptor (FD) blocking flags**. It does not implement the complete POSIX `termios` interface.

PTY master/slave endpoints are documented in the [PTY reference](../devices/PTY.md). The slave uses the same canonical/echo/ONLCR `ioctl()` interface as physical TTYs. `F_SETFL` supports `O_NONBLOCK` on PTY FDs and pipes as well as sockets and physical TTYs.

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

When no input is available, a nonblocking TTY `read()` returns **0** in both canonical and noncanonical modes (not a negative `EAGAIN`-style error). In canonical mode, a line may not yet be complete; Ctrl+D can also produce a zero-length EOF result, so a return value of 0 alone does not distinguish them. Blocking reads wait for input; `VMIN` / `VTIME` timeouts are not available. Pipes and sockets have different I/O semantics; see [Pipes](PIPE.md) and [Socket APIs](../../development/apps/c/NETWORK-PROGRAMMING.md).

## Restoration and screen control

`stty` and `reset` use `OS_TTY_PERSIST` to retain deliberate configuration changes. When a foreground task exits or is interrupted, the kernel restores its saved terminal settings. Interactive programs such as `mb` combine noncanonical input with nonblocking descriptors. `reset` does not change `O_NONBLOCK`.

Cursor visibility, alternate screens, and text attributes are separate from TTY settings. For application screen cleanup, practical `reset` use, and interruption caveats, see [Terminal usage and screen control](TERMINAL.md).

## TTY output newline conversion (ONLCR)

By default, the USB CDC console, UART1, and the Wi-Fi TTY (when enabled) convert LF (`\n`) to CRLF (`\r\n`). An LF immediately following CR is passed through, so existing CRLF sequences are not doubled. Files and non-TTY devices are unaffected.

```text
stty             # Show onlcr or -onlcr
stty -onlcr      # Disable output conversion
stty onlcr       # Enable output conversion
```

C applications can use `ioctl(fd, OS_TTY_GET_ONLCR, 0)` and `ioctl(fd, OS_TTY_SET_ONLCR, 0 or 1)`. The setting is per terminal and independent of canonical input and echo. The `raw` convenience command does not change output conversion.

## Return values of TTY `read()` and `write()`

- `read(fd, buf, n)` returns a positive byte count when input is ready, or 0 when `n=0`. Without available input, a blocking FD waits and a nonblocking FD returns 0. Ctrl+D on an empty canonical line can also return 0.
- `write(fd, buf, n)` returns the number of **input data bytes** accepted, which may differ from bytes transmitted after ONLCR conversion. A full USB CDC transmit queue may cause a zero-byte write. On Wi-Fi TTY, a full queue can return `OS_EAGAIN`, a disconnected terminal can return `OS_EIO`, and a partially accepted write returns the byte count. TTY writes do not automatically block until space becomes available.
- Access to terminal input, or output on a claimed TTY, from a non-owning task may return `OS_EIO`. These rules should not be assumed for ordinary files or sockets.

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


Ctrl+C interrupt interpretation can be queried or configured per TTY with `OS_TTY_GET_INTR` (`0x209`) and `OS_TTY_SET_INTR` (`0x20a`). The default is 1 (terminate the foreground task); 0 delivers byte `0x03` as ordinary input. `pty_test` disables this on its physical console while relaying, then restores it. This is not POSIX signal handling.

**Implementation references:** `tty_canonical`, `tty_echo`, `tty_onlcr`, `tty_saved_*`, `foreground_group`, and `OS_TTY_*` handling in `kernel/kernel.c`; line editing and pipelines in `userland/apps/shell.c`.

---

**Related links:** [TTY guide](TTY.md) | [Guide index](README.md) | [日本語](../../../ja/reference/commands/TTY.md) | [UART1 and terminals](../devices/UART.md) | [C programming](../../development/apps/c/PROGRAMMING.md)
