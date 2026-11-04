# Part C — Advanced: Read and extend UNIX

Read user C code, then boot, scheduling, SVC, VFS, and drivers, before a small extension. Networking is optional.

## Learning story

Do not start by modifying the kernel. Read a small C application in C1, boot and task management in C2, SVC/FD/VFS in C3, and drivers in C4. C5 is optional Pico W networking. C6 plans and verifies a minimal extension, including test evidence and documentation.

**Supporting source guide:** The [kernel reading guide](../../development/kernel/KERNEL-READING-GUIDE.md) provides a **source-reading order**. The textbook provides concepts and exercises; the [function index](../../../FUNCTIONS.md) helps locate functions.

**Exit check:** For one user-visible operation, locate the source path “application → library → SVC → kernel → target.”

## Chapters

- [C1　Start with a user program](C1.md) — Begin with a small C application before changing kernel code
- [C2　Boot, tasks, and scheduling](C2.md) — Trace the kernel path that starts the first user shell
- [C3　Follow SVC, FDs, and VFS in source](C3.md) — Trace a write from user code to its output destination
- [C4　Drivers: from VFS to hardware](C4.md) — Understand the driver boundary before touching network code
- [C5　Optional source tour: Pico W networking](C5.md) **(optional)** — Study lwIP and sockets separately from device drivers
- [C6　Capstone: extend the OS safely](C6.md) — Turn observations and code reading into a testable design

**How to proceed:** Read from C1; you may skip C5 and proceed to C6.

## Prerequisites and next Part

You should understand Part B observations and basic C functions and pointers. Enter from [B5](../B/B5.md).

---

[Back to course contents](../README.md) | [Reference directory](../../../DOCS.md)

---

**Related links:** [日本語](../../../ja/learn/C/README.md)
