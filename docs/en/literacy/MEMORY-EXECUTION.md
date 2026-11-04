# Memory and program execution

[Learning path](LEARNING-PATH.md) | [日本語](../../ja/literacy/MEMORY-EXECUTION.md) | [Memory layout](../reference/MEMORY-LAYOUT.md)

## Goal and prerequisites

**Goal:** Explain Flash/XIP, RAM, task slots, stack, and position-independent code (PIC). Basic hexadecimal addresses help. USB is sufficient. **This lab is read-only: do not write to arbitrary memory addresses.**

## Lab

```text
piconix# ps
piconix# mem -s
piconix# mem
```

In full-screen `mem`, press `1`–`6` for user slots, `k` for kernel RAM, `K` for XIP Flash, and `q` to exit. An unused slot need not contain a running program. Bytes may change while the system is running.

## Observe and explain

- Why does the kernel execute much of its code directly from Flash?
- How does PIC allow a program to run in different RAM slots?
- How can `ps` task data be related to the slots shown by `mem`?

## Next step

Read the [memory viewer guide](MEM.md), [memory layout](../reference/MEMORY-LAYOUT.md), and [internals](../reference/INTERNAL.md) to trace how a program image is placed at run time.

---

## Learning navigation

| ← Previous | ↑ Contents | Next → |
| :--- | :---: | ---: |
| [Terminal lab](TERMINALS-LAB.md) | [Intermediate course](INTERMEDIATE.md) | [System monitoring](MONITORING.md) |
