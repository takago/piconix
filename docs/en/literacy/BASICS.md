# 1. UNIX command basics

[Learning path](LEARNING-PATH.md) | [日本語](../../ja/literacy/BASICS.md) | [Command guide](HELP.md)

## Goal and prerequisites

**Goal:** Explain commands, the shell, paths, and directories, and find information independently. Complete [Setup](../setup/SETUP.md), then connect a Pico or Pico W over USB. No external circuit is needed. `piconix#` denotes the PicoNIX shell, not the Linux build PC.

## Lab

```text
piconix# help
piconix# pwd
piconix# ls /
piconix# ls /bin
piconix# ls /dev
piconix# ls /proc
piconix# ls /tmp
```

`/bin` exposes commands, `/dev` represents devices, `/proc` exposes live OS information, and `/tmp` is a writable RAM-based area. Not all paths correspond to persistent stored files.

## Observe and explain

- How does `pwd` differ from `ls /`?
- Why are `/dev` and `/proc` not ordinary saved files?
- Can you distinguish a PicoNIX shell command from a command run on your Linux PC?

## Next step

Continue with [Files and standard I/O](IO-IPC.md) and save a command's output in a file.
