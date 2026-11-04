# PicoNIX documentation directory

[Project overview](../README.md) | [日本語](DOCS.ja.md) | [Development diary](DEVELOPMENT_DIARY.md)

PicoNIX documentation is organized by **purpose: textbook, supplementary guides, technical reference, and development guides**. Each page identifies its document type near the top.

## 1. Learn UNIX in sequence (textbook)

**[Complete textbook contents: Parts A, B, and C](en/learn/README.md)**

| Part | Learning story | Start here |
|---|---|---|
| **Part A (beginner)** | Shell → files and pipes → processes | [A1 Entering PicoNIX](en/learn/A/A1.md) |
| **Part B (intermediate)** | FDs/VFS → TTY → memory → syscall observation → devices → networking (optional) | [B1 FDs and VFS](en/learn/B/B1.md) |
| **Part C (advanced)** | User C → boot/scheduling → SVC/VFS → drivers → networking (optional) → extension | [C1 User program](en/learn/C/C1.md) |

Textbook sections have labels such as `A1.1`, and each chapter has **Previous / Part contents / Next** navigation. Pico W chapters are optional.

## 2. Find documentation by goal (task index)

**[Task index: “I want to…” → related pages](en/TASKS.md)**

Find textbook chapters, command guides, and references for goals such as editing files, tracing a syscall, or adding a device driver. This index is **not** part of the textbook sequence.

## 3. Look up individual operations (usage guides)

**[Usage guides](en/guides/README.md)** cover detailed commands such as help, pipes, terminal settings, trace, the memory viewer, and editors. Sequential foundational labs now belong exclusively to the textbook. These guides **do not have textbook navigation**.

## 4. Look up current behavior (technical reference)

**[Technical reference index](en/reference/README.md)** covers syscalls, FDs, memory layout, device nodes, networking, and limits. It is intended for lookup, not linear study.

Browse by area: [Devices and I/O](en/reference/devices/README.md) | [Hardware / pins / wiring](en/reference/hardware/README.md) | [Networking (Pico W)](en/reference/network/README.md).

## 5. Read implementation instructions (development guides)

**[Development guide index](en/development/README.md)** covers C, Python, assembly, kernel reading, syscalls, drivers, and Pico W kernel networking. Follow [Part C](en/learn/C/README.md) for learning order.

## Setup and history

- [Setup](en/setup/SETUP.md): build, flash, and connect from Linux.
- [Function index (Japanese)](FUNCTIONS.md): locating source functions.
- [Development diary](DEVELOPMENT_DIARY.md): design decisions.
- [Historical notes](history/README.md): older material may no longer match current behavior.

**To resume a course after looking something up, return to your last numbered A/B/C textbook page rather than following reference links.**
