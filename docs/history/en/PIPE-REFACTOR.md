# Pipe readability refactor

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/guides/README.md), [Internals](../../en/reference/INTERNAL.md), [Pipes](../../en/guides/PIPE.md), and [Network programming](../../en/development/apps/c/NETWORK-PROGRAMMING.md).


The pipe ring transfer logic shared by immediate syscalls and blocked-task wakeup is now centralized in `pipe_transfer()`. `pipe_for_dev()` centralizes the descriptor-to-pipe lookup. The scheduler, FD representation, buffer sizes, and public API are unchanged.

This is a source-level cleanup, not proof of runtime correctness. The pipe tests are static guards only. ARM build and board tests are still required.
