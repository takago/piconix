# Pipe stage 3: kernel foundation (not yet a working shell pipeline)

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/guides/README.md), [Internals](../../en/reference/INTERNAL.md), [Pipes](../../en/guides/PIPE.md), and [Network programming](../../en/development/apps/c/NETWORK-PROGRAMMING.md).


Implemented: `os_pipe(int fds[2])`, two statically allocated 256-byte rings, read/write endpoint validation, reference accounting in `dup2`, standard-FD inheritance during `spawn`, and task-exit/close release. Standard descriptors can now be closed. Pipes return EOF after the last writer closes and their buffer empties; writes with no readers return `OS_EIO`.

**Important limitations:** empty reads and full writes currently return `OS_EAGAIN` even in blocking mode. Scheduler suspension and wake-up have not yet been implemented. `cmd1 | cmd2` parsing and execution are not implemented. This archive is an intermediate source checkpoint, not a complete functional release. ARM firmware and hardware testing remain outstanding.
