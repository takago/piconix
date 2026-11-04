# 2. Standard I/O and interprocess communication

[Learning path](LEARNING-PATH.md) | [日本語](../../ja/literacy/IO-IPC.md) | [Pipe reference](PIPE.md)

## Goal and prerequisites

**Goal:** Distinguish standard input (FD 0), output (FD 1), error (FD 2), redirection, and pipes. A USB-connected Pico is sufficient. Files under `/tmp` disappear on reboot.

## Lab

```text
piconix# echo hello
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# cat -n /tmp/hello.txt
piconix# echo >> /tmp/hello.txt
piconix# cat -n /tmp/hello.txt
piconix# echo hello | cat
piconix# echo hello | cat -n
piconix# echo hello | grep hello | cat
piconix# lsof
```

`cat` reads from a named file, or from standard input when no file is given. `cat -n` adds a right-aligned number and a tab before each line, including blank lines. The added `echo` writes an empty line to the file. Compare the two `cat -n` outputs: the original file remains unchanged by `cat`, but the file appended to by `echo` gains a second line. Line numbering starts at 1 for each invocation.

`>` directs a command's standard output into a file. `|` sends bytes from one command to another. PicoNIX supports at most three commands in a pipeline and restricts mixing pipes with redirection. This is not a complete Linux/POSIX shell.

## Observe and explain

- What is the input source for `cat /tmp/hello.txt` versus `echo hello | cat`?
- Why can you reread a file but not a consumed pipe's data?
- Why might the short-lived `echo` process not appear in `lsof`?

## Next step

Explore `pipe()`, `dup2()`, and EOF in the [Pipe reference](PIPE.md), then move to [Processes](PROCESSES.md).
