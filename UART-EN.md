# UART1 and terminal operation

[日本語](UART-JP.md) | [Overview](README-EN.md) | [Internals](INTERNAL-EN.md)

PicoNIX distinguishes between using `/dev/uart1` as a **raw serial device** and attaching it as an **interactive terminal** through `getty`. This keeps control bytes available for ordinary communication while allowing a second terminal to run a shell or Python.

## 1. Connecting to UART1

Configure UART1 for 115200 baud, 8 data bits, no parity and 1 stop bit (8N1). Verify the RP2040 UART1 pin mapping and electrical levels, and connect a common ground. Use a serial adapter supporting 3.3 V logic.

Enable local echo in the terminal program for interactive UART1 use. UART1 does not provide the same kernel-side line editing as the USB console.

## 2. Starting a shell or Python on another terminal

From the USB `sh`, run:

```sh
getty /dev/uart1 sh &
```

This starts another `sh` on UART1. `getty` is a shell built-in, **not** a separate task. The launched shell receives its own PID and RAM slot. Without the trailing `&`, the originating shell waits for it to exit.

To launch Python directly in interactive mode:

```sh
getty /dev/uart1 python &
```

`getty` attaches stdin (FD 0), stdout (FD 1), and stderr (FD 2) to the requested terminal and assigns terminal affiliation. Child tasks normally inherit both the standard descriptors and terminal affiliation. Starting another `getty` on a terminal already in use is rejected.

## 3. Raw UART versus terminal mode

| Feature | Ordinary `/dev/uart1` communication | Terminal attached by `getty` |
|---|---|---|
| Input | Raw received bytes | Input read by the interactive task |
| Ctrl+C (`0x03`) | Ordinary received byte | Interrupts the terminal's foreground task |
| Ctrl+D (`0x04`) | Ordinary received byte | Reports EOF through `read()` |
| Line input/editing | Byte stream | Application assembles input lines |

Ordinary redirection does **not** change terminal affiliation. For example, `python < /dev/uart1 > /dev/uart1 &` redirects I/O, but does not assign a controlling terminal as `getty` does.

## 4. Ctrl+C and Ctrl+D

**Ctrl+C:** When UART1 is attached as a terminal, the kernel directly terminates its registered foreground task. This is not POSIX `SIGINT` delivery. Pending UART1 input is also cleared.

**Ctrl+D:** Only while UART1 is attached by `getty`, the byte `0x04` is interpreted as EOF. Already collected input is returned first; a subsequent `read()` returns zero. The UART1 `sh` stays active and displays a new prompt on EOF; the Python REPL also stays active and displays another prompt. Use `exit()` to end the session.

For Python started with `getty /dev/uart1 python &`, `exit()` prints `PicoNIX: Session closed.` before quitting. Script execution is unchanged.

When UART1 has no terminal owner, `0x04` remains an ordinary received byte, allowing raw or binary protocols to use it unchanged.

## 5. Inspecting terminals and tasks

Use `tty` to display the current shell's terminal. `ps` and `/proc/tasks` show each task's affiliation in the `TTY` column.

```sh
tty
ps
cat /proc/tasks
```

`tty` prints `/dev/console` in the USB shell and `/dev/uart1` in the UART1 shell.

## 6. Limitations

- There is no full POSIX TTY subsystem, terminal mode switching, signal handlers, or process groups.
- UART1 does not use the USB console's kernel-side line editor.
- Avoid attaching multiple interactive readers to the same terminal.
- `getty` does not perform login authentication.
- Actual behavior also depends on terminal software and serial connection settings.

See also [Setup](SETUP-EN.md), [Internals](INTERNAL-EN.md), and [Python-like interpreter](PYTHON-EN.md).
