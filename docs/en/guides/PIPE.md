# Pipes and standard I/O

*Usage guide*

The PicoNIX shell supports `|` to connect one command's standard output to the next command's standard input. `grep` selects lines containing a literal string.

## Examples

```sh
cat test.s | grep movs
echo hello | grep hello | cat
cat test.s | grep movs | hexdump
```

Pipelines may contain two or three commands; four or more stages are not supported. `grep` performs simple substring matching, not regular-expression matching.

## Implementation

- `pipe(fds)` returns a read endpoint in `fds[0]` and a write endpoint in `fds[1]`.
- The shell saves stdin/stdout using `dup()` and connects FD 0 (stdin) and FD 1 (stdout) with `dup2()`.
- Child tasks inherit descriptors on spawn. The shell closes unused endpoints and restores its standard descriptors afterward.
- After all writers close and buffered data is consumed, reading returns EOF (zero bytes). Writing with no readers returns an error.
- Reading from an empty pipe or writing to a full pipe waits for progress. The internal implementation has an `O_NONBLOCK` branch returning `OS_EAGAIN`, but the current `fcntl(F_SETFL)` does not support pipe descriptors. Therefore applications cannot enable nonblocking pipes through the public API.

## Limits and shell restrictions

Pipelines cannot be combined with `&`, `<`, or `>`; use those operators with standalone commands.


| Item | Limit |
|---|---|
| Concurrent pipes | 2 |
| Buffer per pipe | 256-byte ring |
| File descriptors per task | 16 (0–15) |
| Shell pipeline length | Up to 3 commands |
| Buffer allocation | Static; no heap allocation |

Pipes carry byte streams and are not stored as files under `/tmp`. The shell interprets `|`; individual commands do not need to parse it.

This is a deliberately small UNIX-like implementation, not a complete POSIX shell. Complex quoting, combinations of redirection and pipes, and arbitrary-length pipelines are not guaranteed.

## Ctrl+C and network pipelines

Ctrl+C interrupts registered foreground pipeline tasks (up to three stages) directly in the kernel, without POSIX signal delivery. For one-shot UDP transmission such as `echo hello | nc -u -N 192.168.4.2 12345`, see the [nc guide](../reference/network/NC.md).

**Implementation references:** `run_pipeline()` and command parsing in `userland/apps/shell.c`; `PIPE_COUNT`, `PIPE_BYTES`, `pipe_transfer()`, and pipe syscalls in `kernel/kernel.c`.

---

**Related links:** [Pipe guide](PIPE.md) | [Guide index](README.md) | [日本語](../../ja/guides/PIPE.md)
