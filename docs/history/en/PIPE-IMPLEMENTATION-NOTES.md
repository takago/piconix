# Pipe implementation audit (work in progress)

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/literacy/README.md), [Internals](../../en/reference/INTERNAL.md), [Pipes](../../en/literacy/PIPE.md), and [Network programming](../../en/development/apps/c/NETWORK-PROGRAMMING.md).


This snapshot adds the independent `grep` command only. `pipe()` and shell `|` are NOT implemented yet.

Audit findings:
- `task_t` stores 8 integer device descriptors plus positions, flags and target PIDs.
- `spawn` copies only descriptors 0, 1 and 2 to a child; descriptor references are not counted.
- `SYS_DUP2` overwrites the destination descriptor without closing its previous endpoint.
- `SYS_CLOSE` rejects descriptors 0, 1 and 2.
- `finish_task` does not walk open descriptors to release them.
- TTY blocking reads are completed in `wake_readers` only after `wait_frame_ready`.
- `wake_waiters` similarly handles waitpid completion after saved frame availability.

Next kernel stage should introduce fixed pipe slots, reference counts, shared close/release logic,
read/write wait state with PendSV-safe completion, and safe child descriptor inheritance.
Only then should the shell launch both sides concurrently and close its own pipe ends.
