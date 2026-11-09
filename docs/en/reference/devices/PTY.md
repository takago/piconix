# Pseudoterminal (PTY)

PicoNIX provides **one general-purpose master/slave PTY pair**, independent of MCP. The MCP terminal tools have **not yet been implemented** in this stage.

## Device nodes

| Path | Open mode | Purpose |
|---|---|---|
| `/dev/ptmx` | `open(..., O_RDWR)` | Exclusive master; a second open returns `OS_EBUSY` |
| `/dev/pts/0` | `open(..., O_RDWR)` or other appropriate mode | Slave; available while the master is open |

- Writing to the master supplies terminal input to the slave, including canonical input, echo, and Ctrl+C handling. Reading the master receives slave output.
- The slave is TTY number 3 and supports the standard PicoNIX TTY `ioctl()` commands and `fcntl(O_NONBLOCK)`. `tty` reports `/dev/pts/0`.
- `spawn()` inherits FDs 0–2. If the parent's FD 0 points to the slave, the **child** receives PTY as its controlling TTY. Use `spawn_foreground()` to establish foreground ownership; the parent's controlling TTY remains unchanged.
- No additional process or network socket is required. The slave-to-master output ring is 1024 bytes; ANSI screen emulation remains a user-space responsibility.
- A blocking slave write waits when the output ring is full; with `O_NONBLOCK` it returns `OS_EAGAIN`. Partial writes are possible. A full master-to-slave input queue causes the master's write to return `OS_EAGAIN`; the caller must retry.
- Once all slave FDs close, the master drains remaining output and then returns EOF (0). When all master FDs close, the slave drains pending input, then reads EOF and writes return `OS_EIO`. The state resets after both ends close.
- A new pair cannot be allocated until the previous session has closed both ends. FDs can be duplicated with `dup()` and `dup2()`.

## Typical sequence

1. Open the master with `O_RDWR`, then open the slave.
2. Save the parent's FDs 0–2 using `dup()`; redirect them to the slave using `dup2()`, and start the child via `spawn_foreground()`.
3. Restore the parent's standard FDs, and close its slave FD.
4. Write keys to the master and read ANSI output from it, using `select()` if needed.
5. Use `waitpid()` to collect the child and close the master.

Full-screen applications such as `vi` emit ANSI escapes. The master-side terminal emulator, not the kernel, must interpret them. Host-side PTY helper tests pass, but device testing with `vi` is still required.


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
cleanup. Use `reset` or `stty` if needed. ANSI screen emulation and MCP
terminal tools are not yet implemented.

`OS_TTY_GET_INTR` (`0x209`) and `OS_TTY_SET_INTR` (`0x20a`) control whether a
TTY interprets Ctrl+C as an interrupt. The default is 1 (enabled); 0 passes
`0x03` as an ordinary input byte. `pty_test` restores the setting on exit.

**Source:** `pty_ref()`, `pty_master_rw()`, `pty_slave_write()`, `wake_ptys()`, `spawn_in_slot()`, `foreground_child()` in `kernel/kernel.c`; names in `include/device_names.h`. Regression: `tools/test_scripts/test_pty_runtime.py`.

[Devices](README.md) | [TTY](../commands/TTY.md) | [日本語](../../../ja/reference/devices/PTY.md)

