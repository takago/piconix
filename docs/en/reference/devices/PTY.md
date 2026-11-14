# Pseudoterminal (PTY)

PicoNIX provides **up to two general-purpose master/slave PTY pairs**, independent of MCP. MCP clients can also use the PTY through its virtual screen and named-key devices.

## Generic PTY screen-only mode

`mcpd` starts an interactive shell on its PTY at startup. After opening the master but before opening the slave, it enables `ioctl(master, OS_PTY_SCREEN_ONLY, 1)`. Slave output updates the assigned `/dev/pts/N.screen` display directly without accumulating bytes in the 1024-byte output ring. No output drain or fixed 200 ms wait is required. Raw `read()` from the master is not available in this mode. Normal PTY use (e.g. `pty_test`) retains the output ring and master reads. The mode resets when both endpoints close.

`terminal_send` returns only the accepted byte count; the LLM explicitly calls `terminal_read` to inspect the screen. Screen reads are not atomic snapshots.

## Device nodes

| Path | Open mode | Purpose |
|---|---|---|
| `/dev/ptmx` | `open(..., O_RDWR)` | Allocates the first free pair (0 or 1); a third open returns `OS_EBUSY` |
| `/dev/pts/0` | `open(..., O_RDWR)` or other appropriate mode | Slave; available while the master is open |
| `/dev/pts/0.screen` | `O_RDONLY` | 80x24 screen with ANSI inverse/cursor markers (variable-length stream) |
| `/dev/pts/0.key` | `O_WRONLY` | Prefixed `key:` or `text:` input |
| `/dev/pts/1`, `/dev/pts/1.screen`, `/dev/pts/1.key` | As above | Second slave, screen, and named-key input |

- Writing to the master supplies terminal input to the slave, including canonical input, echo, and Ctrl+C handling. Reading the master receives slave output.
- The slaves use TTY numbers 3 (PTY 0) and 5 (PTY 1) and support the standard PicoNIX TTY `ioctl()` commands and `fcntl(O_NONBLOCK)`. `tty` reports the corresponding `/dev/pts/N`.
- `spawn()` inherits FDs 0–2. If the parent's FD 0 points to the slave, the **child** receives PTY as its controlling TTY. Use `spawn_foreground()` to establish foreground ownership; the parent's controlling TTY remains unchanged.
- No additional process or network socket is required. The slave-to-master output ring is 1024 bytes per pair; The kernel provides an intentionally limited ANSI-interpreted text screen; raw PTY output is unchanged.
- A blocking slave write waits when the output ring is full; with `O_NONBLOCK` it returns `OS_EAGAIN`. Partial writes are possible. A full master-to-slave input queue causes the master's write to return `OS_EAGAIN`; the caller must retry.
- Once all slave FDs close, the master drains remaining output and then returns EOF (0). When all master FDs close, the slave drains pending input, then reads EOF and writes return `OS_EIO`. The state resets after both ends close.
- A pair can be reused after both its master and slave references are closed. Only allocated pairs appear in `ls /dev/pts`; the slave nodes are not listed before allocation. FDs can be duplicated with `dup()` and `dup2()`.


### Discovering the allocated slave

Every `open("/dev/ptmx", ...)` allocates the lowest available slot, 0 or 1. Use the new master-only ioctl before opening the slave; do not hardcode `/dev/pts/0`:

```c
int master = open("/dev/ptmx", O_RDWR | O_NONBLOCK);
int n = ioctl(master, OS_PTY_GET_INDEX, 0); /* 0 or 1 */
char path[32];
snprintf(path, sizeof path, "/dev/pts/%d", n);
int slave = open(path, O_RDWR);
```

`OS_PTY_GET_INDEX` (`0x20c`) is available on master FDs only. Both `mcpd` and `pty_test` use it. A master close hides that slot's nodes; lingering slave FDs delay reuse until they are closed.

## Typical sequence

1. Open `/dev/ptmx` with `O_RDWR`, obtain the slot with `ioctl(master, OS_PTY_GET_INDEX, 0)`, then open `/dev/pts/N` for that slot.
2. Save the parent's FDs 0–2 using `dup()`; redirect them to the slave using `dup2()`, and start the child via `spawn_foreground()`.
3. Restore the parent's standard FDs, and close its slave FD.
4. Write keys to the master and read ANSI output from it, using `select()` if needed.
5. Use `waitpid()` to collect the child and close the master.

Full-screen applications such as `vi` emit ANSI escapes. The master still receives unmodified ANSI output. The `.screen` device also provides a minimally interpreted current screen.


## On-device test (standard command)

`pty_test` (application ID 56) is included in standard firmware builds. Its source is `userland/apps/pty_test.c`; no extra build option is needed.

```text
piconix# pty_test
/dev/pts/0  (or /dev/pts/1 if slot 0 is in use)
```

With no arguments, `pty_test` runs `tty` on the slave. With a command, it connects the command's standard I/O to the slave and relays input and output between the current console and the master.

```text
piconix# pty_test fd
piconix# pty_test vi /tmp/test.txt
piconix# pty_test tty
/dev/pts/0  (or /dev/pts/1 if slot 0 is in use)
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
echo 'key:DOWN' > /dev/pts/0.key
echo 'key:CTRL_C' > /dev/pts/0.key
echo 'text:ls\n' > /dev/pts/0.key
```

The screen and key nodes require an open PTY master. MCP `terminal_read` forwards the screen with its embedded SGR 4/24 cursor marker; no separate cursor-coordinate device is provided. Visible cursor movement changes the screen data. CSI ?25l hides the cursor marker; CSI ?25h restores it. The screen is fixed at 80 columns and 24 rows, with no scrollback. Each row contains 80 character cells plus LF, with optional ANSI SGR sequences. `ESC[7m` and `ESC[0m` mark reversed cells; `ESC[4m` and `ESC[24m` mark the single cursor cell (underline on/off), including a blank cell, but emit no marker when the cursor is hidden. Reads advance the FD offset and may split an escape sequence. The display is not an atomic snapshot if the screen changes while reading. The cursor marker is synthesized on read, not stored as a cell attribute.

`.key` accepts one prefixed `key:` or `text:` command per `write()`; see the input protocol below. The PTY master FD also accepts ordinary text directly.

This is a deliberately minimal ANSI interpreter, supporting basic cursor movement, erasure, reverse video, alternate-screen switching, and CSI 6n. It does not implement a full terminal, multibyte glyph layout, or resizes. PTY byte forwarding remains unchanged. See [MCP](../commands/MCP.md) for remote access.

## `.key` input protocol

Each `write()` carries one command and must start with `key:` or `text:`. Bare legacy key names are rejected.

```sh
echo 'key:CTRL_C' > /dev/pts/0.key
echo 'key:ENTER' > /dev/pts/0.key
echo 'text:ls\n' > /dev/pts/0.key
```

`key:` accepts one printable ASCII character or the existing named keys (`UP`, `DOWN`, `ENTER`, `ESC`, `CTRL_A` through `CTRL_Z`, etc.). `text:` accepts literal text with `\n` representing LF and `\\` representing a backslash. The final LF appended by `echo` (and a preceding CR) terminates the command; it is not sent to the slave. Output is limited to 127 decoded bytes per write. Writes are independent commands and cannot be split across calls. Invalid commands are rejected before any input is sent; PTY backpressure can still result in a partial transfer. The MCP `terminal_send` interface remains unchanged.
