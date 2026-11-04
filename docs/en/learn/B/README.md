# Part B — Intermediate: Observe UNIX

Explain your observations through FDs, TTYs, memory, syscalls, and devices. Networking is optional and requires Pico W.

## Learning story

You will now observe the OS beneath the commands learned in Part A. B1 introduces FDs and the VFS; B2 covers TTYs; B3 examines memory; B4 records syscalls; and B5 examines devices. B6 is optional for Pico W networking.

**New observation tools:** `mem` and `trace` are not ordinary file commands. Before B3, check the [mem guide](../../guides/MEM.md); before B4, check the [trace guide](../../guides/TRACE.md). The corresponding chapters also explain how to read their results.

**Exit check:** Explain `echo hello | cat` using FDs, pipes, and syscalls, not only by describing what appears on the screen.

## Chapters

- [B1　File descriptors and VFS](B1.md) — Understand how files, terminals, and pipes share an I/O interface
- [B2　TTY and terminal I/O](B2.md) — Separate terminal input settings from screen rendering
- [B3　Flash, RAM, and tasks](B3.md) — Locate stored program images and running code
- [B4　Observe system calls with trace](B4.md) — See the boundary between a user program and the kernel
- [B5　Devices as files](B5.md) — Compare device I/O with ordinary files
- [B6　Optional lab: Pico W networking](B6.md) **(optional)** — Extend the device story to sockets and user-space servers

**How to proceed:** Follow the required chapters. You can skip B6 and continue to C1.

## Prerequisites and next Part

Basic command-line skills from Part A are expected; review [A4](../A/A4.md). Continue with [C1](../C/C1.md).

---

[Back to course contents](../README.md) | [Reference directory](../../../DOCS.md)

---

**Related links:** [日本語](../../../ja/learn/B/README.md)
