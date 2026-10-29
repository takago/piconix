# Terminal Settings and Interactive Input

[日本語](TTY-EN.md) | [Overview](README-EN.md) | [UART1 and terminals](UART-EN.md) | [C programming](PROGRAMMING-EN.md)

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

Terminal modes are shared by users of a terminal. An interactive application should restore its previous settings so that the shell remains usable. For terminal affiliation, `getty`, Ctrl+C/Ctrl+D, and USB CDC versus UART1, see [UART1 and terminals](UART-EN.md).
