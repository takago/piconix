# Pipe integration: implementation gate

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/guides/README.md), [Internals](../../en/reference/INTERNAL.md), [Pipes](../../en/guides/PIPE.md), and [Network programming](../../en/development/apps/c/NETWORK-PROGRAMMING.md).


This stage documents the baseline architecture. It does **not** implement `pipe()`.

- `task_t` owns eight FD slots; `spawn_in_slot` copies descriptors 0..2 into the child.
- `SYS_DUP2` currently copies FD metadata without reference accounting.
- `SYS_CLOSE` refuses descriptors 0..2; pipeline support must allow their release.
- `finish_task` does not release all descriptors.
- The shell uses `spawn` and `waitpid`; a pipeline must spawn both peers before waiting.

## Required invariants before enabling `|`

1. A pipe endpoint's read/write reference counts include all `dup2` aliases and inherited descriptors.
2. Closing the final writer wakes blocked readers, which subsequently return EOF (zero bytes).
3. Closing the final reader wakes blocked writers, which return an error rather than deadlocking.
4. Pipe read/write suspension uses the same safe context-switch completion rules as existing blocking I/O.
5. Task exit closes all descriptors, including 0..2; `dup2(x,x)` is a no-op.
6. `cmd1 | cmd2` launches both children before waiting, and restores shell descriptors on every error path.
7. Socket FD ownership semantics must remain unchanged.

Do not mark the pipe complete based on the static contract checks alone. Compile firmware and exercise the RP2040 tests.
