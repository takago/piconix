# Terminal Settings and Interactive Input

[日本語](TTY.md) | [Overview](README.md) | [UART1 and terminals](UART.md) | [C programming](PROGRAMMING.md)

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

C applications can use `ioctl()` with `OS_TTY_GET_CANON`, `OS_TTY_GET_ECHO`, `OS_TTY_SET_CANON`, and `OS_TTY_SET_ECHO`. `stty` calls `OS_TTY_PERSIST` to retain settings after it exits. Interactive applications should generally change settings temporarily and restore them on exit. `VMIN` and `VTIME` are not supported.

## Nonblocking I/O: `fcntl()`

Use `fcntl(fd, F_GETFL, 0)` to read FD flags, and `fcntl(fd, F_SETFL, flags)` to change `O_NONBLOCK`. This is independent of canonical and echo modes. For immediate character input, select noncanonical mode and an appropriate blocking policy.

## `mb` (moonbuggy)

```text
mb
```

Press SPACE to jump and `q` to quit. `mb` temporarily uses character-based, no-echo, nonblocking input and combines `read()` with `msleep()` for interaction and updates. On normal exit, it restores the saved terminal settings and input FD flags.

## `reset` (restore terminal display)

```text
reset
```

`reset` enables `icanon` and `echo` on standard input and emits ANSI escape sequences to reset character attributes, restore the cursor, clear the display, and move home. It does not change `O_NONBLOCK`. It cannot be used if input is no longer possible.

## Notes and related guides

Terminal modes are shared by users of a terminal. An interactive application should restore its previous settings so that the shell remains usable. For terminal affiliation, `getty`, Ctrl+C/Ctrl+D, and USB CDC versus UART1, see [UART1 and terminals](UART.md).

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
