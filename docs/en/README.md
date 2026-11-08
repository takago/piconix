# PicoNIX documentation directory

**Audience:** Readers who already know UNIX/Linux operations, basic OS concepts and C. These documents explain how PicoNIX implements those ideas, not the underlying theory. See the [prerequisites and roadmap](ROADMAP.md#audience-and-prerequisites).

PicoNIX documentation is organized into **setup, technical reference, and development**. For a suggested learning order, use the [Learning roadmap](ROADMAP.md).

## Start here: Setup

**[Setup guide](setup/SETUP.md)**: Prepare the Linux build environment, flash Pico or Pico W, and connect through USB.

## 1. Learn in order (roadmap)

**[Learning roadmap](ROADMAP.md)** gives a suggested path through command references, OS observation, and source-code development.

## 2. Find documentation by goal (task index)

**[Task index: “I want to…” → related pages](TASKS.md)**

Find learning steps, command references and development documents for goals such as editing files, tracing a syscall, or adding a device driver. This index is **not** part of the learning roadmap sequence.


## 3. Look up current behavior (technical reference)

**[Technical reference index](reference/README.md)** covers syscalls, FDs, memory layout, device nodes, networking, and limits. It is intended for lookup, not linear study.

Browse by area: [Commands](reference/commands/README.md) | [Devices and I/O](reference/devices/README.md) | [Hardware / pins / wiring](reference/hardware/README.md) | [Networking (Pico W)](reference/network/README.md).

## 4. Read implementation instructions (development guides)

**[Development guide index](development/README.md)** covers C, Python, assembly, kernel reading, syscalls, drivers, and Pico W kernel networking. Follow [Learning roadmap](ROADMAP.md) for learning order.

## Source index and development records

- [Function index (Japanese)](../ja/development/FUNCTIONS.md): locating source functions.
- [Development diary](DEVELOPMENT_DIARY.md): design decisions.
For previous design decisions and changes, see the [development diary](DEVELOPMENT_DIARY.md) and Git commit history.

---

[Project overview](../../README.md) | [日本語](../ja/README.md) | [Development diary](DEVELOPMENT_DIARY.md)
