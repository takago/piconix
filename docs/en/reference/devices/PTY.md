# Pseudoterminal (PTY)

PicoNIX provides **one general-purpose master/slave PTY pair**, independent of MCP. MCP clients can also use the PTY through its virtual screen and named-key devices.

## Device nodes

| Path | Open mode | Purpose |
|---|---|---|
| `/dev/ptmx` | `open(..., O_RDWR)` | Exclusive master; a second open returns `OS_EBUSY` |
| `/dev/pts/0` | `open(..., O_RDWR)` or other appropriate mode | Slave; available while the master is open |
| `/dev/pts/0.screen` | `O_RDONLY` | 80x24 screen with ANSI inverse/cursor markers (variable-length stream) |
| `/dev/pts/0.key` | `O_WRONLY` | Named-key input (e.g. `DOWN`, `CTRL_C`) |

- Writing to the master supplies terminal input to the slave, including canonical input, echo, and Ctrl+C handling. Reading the master receives slave output.
- The slave is TTY number 3 and supports the standard PicoNIX TTY `ioctl()` commands and `fcntl(O_NONBLOCK)`. `tty` reports `/dev/pts/0`.
- `spawn()` inherits FDs 0–2. If the parent's FD 0 points to the slave, the **child** receives PTY as its controlling TTY. Use `spawn_foreground()` to establish foreground ownership; the parent's controlling TTY remains unchanged.
- No additional process or network socket is required. The slave-to-master output ring is 1024 bytes; The kernel provides an intentionally limited ANSI-interpreted text screen; raw PTY output is unchanged.
- A blocking slave write waits when the output ring is full; with `O_NONBLOCK` it returns `OS_EAGAIN`. Partial writes are possible. A full master-to-slave input queue causes the master's write to return `OS_EAGAIN`; the caller must retry.
- Once all slave FDs close, the master drains remaining output and then returns EOF (0). When all master FDs close, the slave drains pending input, then reads EOF and writes return `OS_EIO`. The state resets after both ends close.
- A new pair cannot be allocated until the previous session has closed both ends. FDs can be duplicated with `dup()` and `dup2()`.

## Typical sequence

1. Open the master with `O_RDWR`, then open the slave.
2. Save the parent's FDs 0–2 using `dup()`; redirect them to the slave using `dup2()`, and start the child via `spawn_foreground()`.
3. Restore the parent's standard FDs, and close its slave FD.
4. Write keys to the master and read ANSI output from it, using `select()` if needed.
5. Use `waitpid()` to collect the child and close the master.

Full-screen applications such as `vi` emit ANSI escapes. The master still receives unmodified ANSI output. The `.screen` device also provides a minimally interpreted current screen.


## On-device test (standard command)

`pty_test` (application ID 56) is included in standard firmware builds. Its source is `userland/apps/pty_test.c`; no extra build option is needed.

```text
piconix# pty_test
/dev/pts/0
```

With no arguments, `pty_test` runs `tty` on the slave. With a command, it connects the command's standard I/O to the slave and relays input and output between the current console and the master.

```text
piconix# pty_test fd
piconix# pty_test vi /tmp/test.txt
piconix# pty_test tty
/dev/pts/0
```

During the relay, the physical terminal is switched to noncanonical input with echo disabled. `select()` monitors the console and PTY master; terminal settings and descriptors are restored on exit. Output is drained until the slave closes.

**Ctrl+C:** During a relay, `pty_test` disables Ctrl+C interrupt handling on the
physical console and forwards byte `0x03` to the PTY master. The PTY slave
terminates the foreground task using PicoNIX's existing termination path,
not POSIX `SIGINT`. With `pty_test sh`, interrupting a foreground `fd` should
leave the PTY shell running. Forced termination does not run application
cleanup. Use `reset` or `stty` if needed. Basic ANSI screen emulation and MCP
terminal tools are available.

`OS_TTY_GET_INTR` (`0x209`) and `OS_TTY_SET_INTR` (`0x20a`) control whether a
TTY interprets Ctrl+C as an interrupt. The default is 1 (enabled); 0 passes
`0x03` as an ordinary input byte. `pty_test` restores the setting on exit.

**Source:** `pty_ref()`, `pty_master_rw()`, `pty_slave_write()`, `wake_ptys()`, `spawn_in_slot()`, `foreground_child()` in `kernel/kernel.c`; names in `include/device_names.h`. Regression: `tools/test_scripts/test_pty_runtime.py`.

[Devices](README.md) | [TTY](../commands/TTY.md) | [日本語](../../../ja/reference/devices/PTY.md)


## Virtual screen and named keys

```sh
cat /dev/pts/0.screen
echo DOWN > /dev/pts/0.key
echo CTRL_C > /dev/pts/0.key
```

The screen and key nodes require an open PTY master. MCP `terminal_read` forwards the screen with its embedded SGR 4/24 cursor marker; no separate cursor-coordinate device is provided. Visible cursor movement changes the screen data and therefore its change-detection hash. CSI ?25l hides the cursor marker; CSI ?25h restores it. The screen is fixed at 80 columns and 24 rows, with no scrollback. Each row contains 80 character cells plus LF, with optional ANSI SGR sequences. `ESC[7m` and `ESC[0m` mark reversed cells; `ESC[4m` and `ESC[24m` mark the single cursor cell (underline on/off), including a blank cell, but emit no marker when the cursor is hidden. Reads advance the FD offset and may split an escape sequence. The display is not an atomic snapshot if the screen changes while reading. The cursor marker is synthesized on read, not stored as a cell attribute.

`.key` accepts one named key per `write()`: `UP`, `DOWN`, `LEFT`, `RIGHT`, `ENTER`, `ESC`, `ESCAPE`, `TAB`, `SPACE`, `SP`, `BACKSPACE`, `BS`, `DELETE`, or `CTRL_A` through `CTRL_Z`. Use the master FD for ordinary text.

This is a deliberately minimal ANSI interpreter, supporting basic cursor movement, erasure, reverse video, alternate-screen switching, and CSI 6n. It does not implement a full terminal, multibyte glyph layout, or resizes. PTY byte forwarding remains unchanged. See [MCP](../commands/MCP.md) for remote access.
