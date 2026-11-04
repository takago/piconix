# Terminals and interactive input

[Learning path](LEARNING-PATH.md) | [日本語](../../ja/literacy/TERMINALS-LAB.md) | [TTY reference](TTY.md)

## Goal and prerequisites

**Goal:** Distinguish line input, terminal echo, cursor display, and FD read behavior. A USB terminal is sufficient. First read [terminal recovery](TERMINAL.md) in case display state becomes confused.

## Lab

```text
piconix# tty
piconix# stty
piconix# stty raw
piconix# stty
piconix# stty cooked
piconix# stty
```

PicoNIX `raw` and `cooked` are simplified modes, not complete POSIX `termios` settings. In particular, `raw` disables canonical input and echo. Restore `stty cooked` when finished. `stty` settings and the ANSI sequences used by applications to draw a screen are separate concepts.

## Observe and explain

- What differs between character-by-character and line-based input?
- Are terminal echo and the shell's input rendering the same operation?
- Does an editor drawing the cursor imply the kernel owns screen layout?

## Next step

Compare `ioctl()` and `fcntl()` in the [TTY reference](TTY.md), and study alternate screens and `reset` in [Terminal display](TERMINAL.md).

---

## Learning navigation

| ← Previous | ↑ Contents | Next → |
| :--- | :---: | ---: |
| [Intermediate course](INTERMEDIATE.md) | [Intermediate course](INTERMEDIATE.md) | [Memory and execution](MEMORY-EXECUTION.md) |
