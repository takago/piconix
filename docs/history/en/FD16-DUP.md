# FD table and descriptor allocation

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/README.md), [Internals](../../en/INTERNAL.md), [Pipes](../../en/PIPE.md), and [Network programming](../../en/NETWORK-PROGRAMMING.md).


- Per-task FD limit: 16 (0..15); user allocation starts at 3.
- `dup(oldfd)` (syscall 37) returns the lowest unused FD, or a negative error.
- `dup2(oldfd,newfd)` remains available for standard-stream redirection.
- Shell redirection and two/three-stage pipelines use `dup()` rather than fixed FD 6/7.
- Pipe reference counts are incremented on duplication and released on close/exit.
- Socket duplication remains unsupported, matching the existing `dup2` restriction.
- No change to 2 x 256-byte pipe buffers or 32-KiB task slots.
- Firmware build and hardware runtime tests are still required.
