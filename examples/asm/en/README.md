# Thumb Assembly Examples

These examples target PicoNIX's tiny assembler. Each `.s` file is at most 1,024 bytes. Copy a source file into `/tmp`, then run `as filename` followed by `./a.out`. Each assembly overwrites `/tmp/a.out`.

| File | Demonstrates |
|---|---|
| `01-exit.s` | `exit(0)` via `svc #0` |
| `02-hello.s` | String output using `adr` and `write` |
| `03-stack.s` | Saving/restoring registers with `push` and `pop` (no output) |
| `04-print-int.s` | Converting the `getpid` return value (0–255) to decimal ASCII, then calling `write` |

## How the number example works

`04-print-int.s` repeatedly subtracts 100 and 10 to split a number in `r0` into decimal digits, stores ASCII bytes via `strb` in a three-byte `.space` buffer, then calls `write`. The label `print_int` marks its subroutine. Descriptive labels, whitespace, and comments aid readability.

This example is **not** a general-purpose 32-bit `itoa`: it handles only integers from 0 to 255, not negative or larger values. `.space` only reserves writable bytes in the program image; it does not provide memory management.

**Caution:** The Cortex-M0+ does not provide general user-process memory protection. Incorrect assembly may crash the OS.

## Source formatting

Labels begin at the first column. Tabs separate instructions and operands; the shorter examples also indent instructions with tabs. `04-print-int.s` now also indents instructions with tabs and uses meaningful labels, blank lines, and brief comments.

## Program termination

All examples terminate their main path via `exit(0)`: set `r0 = 3` (syscall number), `r1 = 0` (exit status), then execute `svc #0`. SVC transfers control to the kernel; its immediate `#0` is not the syscall number. No example depends on `bx lr` to terminate its entry point.

In `04-print-int.s`, `pop {r4,r5,r6,r7,pc}` **returns from the `print_int` subroutine** to the caller; it does not terminate the program.
