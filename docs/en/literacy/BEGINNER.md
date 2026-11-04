# Beginner course — use PicoNIX

[Learning path](LEARNING-PATH.md) | [Intermediate](INTERMEDIATE.md) | [日本語](../../ja/literacy/BEGINNER.md)

**Course route:** [Command basics](BASICS.md) → [Standard I/O and pipes](IO-IPC.md) → [Processes](PROCESSES.md) → [Intermediate course](INTERMEDIATE.md). Use Next at the end of each lesson. Supplementary topics and the Pico W networking lab are optional.

**Goal:** Find shell commands, create RAM files, connect standard I/O, and inspect running processes. No kernel source knowledge is required.

**Before starting:** Complete [Setup](../setup/SETUP.md) on a Pico or Pico W and connect a USB terminal. All commands here run on PicoNIX. `/tmp` is lost on reboot.

## 1. Explore the shell and directories

Read [Basics](BASICS.md) and [help](HELP.md), then run:

```text
piconix# help
piconix# pwd
piconix# ls /
piconix# ls /bin
piconix# ls /dev
piconix# ls /proc
piconix# ls /tmp
```

**Think:** What do `/bin`, `/dev`, and `/proc` represent? Why are not all paths ordinary stored files?

## 2. Create and display a file

```text
piconix# echo hello > /tmp/note.txt
piconix# cat /tmp/note.txt
piconix# cat -n /tmp/note.txt
piconix# cp /tmp/note.txt /tmp/copy.txt
piconix# cat /tmp/copy.txt
```

`>` redirects output to a file. `cat -n` adds display-only line numbers. Try [edit](EDIT.md) or [vi](VI.md) for text editing, and [fd](FD.md) for interactive file browsing.

**Think:** How do `echo`, `cat`, and `cp` differ? Why is a file in `/tmp` lost on reboot?

## 3. Explore standard I/O and pipelines

Use the [standard I/O lab](IO-IPC.md) to compare file input with pipe input:

```text
piconix# echo hello | cat
piconix# echo hello | cat -n
piconix# echo hello | grep hello | cat
piconix# lsof
```

Each side of `|` is a separate command. PicoNIX supports pipelines of up to three commands; it is not a complete Linux shell. See [Pipes](PIPE.md) for implementation details.

**Think:** How does the input source of `cat /tmp/note.txt` differ from `echo hello | cat`? Explain using stdin and stdout.

## 4. Inspect processes

Read the [process lab](PROCESSES.md) and run:

```text
piconix# ps
piconix# blink &
piconix# ps
piconix# cat /proc/tasks
```

`&` starts a background task. Find its **actual PID** in `ps` and terminate it with `kill PID` (replace `PID` with the number). Task slots are limited; do not leave unused background tasks running.

## Check your understanding

1. Find help for an unfamiliar command.
2. Create and read a `/tmp` file; explain the difference between `>` and `|`.
3. Explain why a PID is not the same as a program name under `/bin`.
4. Explain the distinct roles of `/dev`, `/proc`, and `/tmp`.

Then move to the [intermediate course](INTERMEDIATE.md). For optional exploration, see [dd](DD.md), [Pong](PONG.md), or [Python](../development/apps/python/PYTHON.md).

---

## Learning navigation

| ← Previous | ↑ Contents | Next → |
| :--- | :---: | ---: |
| [Learning path](LEARNING-PATH.md) | [Learning path](LEARNING-PATH.md) | [Command basics](BASICS.md) |
