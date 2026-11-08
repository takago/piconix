# PicoNIX documentation directory

**Audience:** Readers who already know UNIX/Linux operations, basic OS concepts and C. These documents explain how PicoNIX implements those ideas, not the underlying theory. See the [prerequisites and roadmap](en/ROADMAP.md#audience-and-prerequisites).

PicoNIX documentation is organized into **usage guides, technical reference, and development guides**. For a suggested learning order, use the [Learning roadmap](en/ROADMAP.md).

## Start here: Setup

**[Setup guide](en/setup/SETUP.md)**: Prepare the Linux build environment, flash Pico or Pico W, and connect through USB.

## 1. Learn in order (roadmap)

**[Learning roadmap](en/ROADMAP.md)** gives a suggested path through practical guides, OS observation, and development exercises.

## 2. Find documentation by goal (task index)

**[Task index: “I want to…” → related pages](en/TASKS.md)**

Find learning steps, command guides, and references for goals such as editing files, tracing a syscall, or adding a device driver. This index is **not** part of the learning roadmap sequence.

## 3. Look up individual operations (usage guides)

**[Usage guides](en/guides/README.md)** cover detailed commands such as help, pipes, terminal settings, trace, the memory viewer, and editors. The roadmap suggests an order; step-by-step instructions are provided in each guide.

## Exercises

**[Exercise index](en/exercises/README.md)**: investigation and review tasks, separate from command examples in the guides.

## 4. Look up current behavior (technical reference)

**[Technical reference index](en/reference/README.md)** covers syscalls, FDs, memory layout, device nodes, networking, and limits. It is intended for lookup, not linear study.

Browse by area: [Devices and I/O](en/reference/devices/README.md) | [Hardware / pins / wiring](en/reference/hardware/README.md) | [Networking (Pico W)](en/reference/network/README.md).

## 5. Read implementation instructions (development guides)

**[Development guide index](en/development/README.md)** covers C, Python, assembly, kernel reading, syscalls, drivers, and Pico W kernel networking. Follow [Learning roadmap](en/ROADMAP.md) for learning order.

## Source index and development records

- [Function index (Japanese)](FUNCTIONS.md): locating source functions.
- [Development diary](DEVELOPMENT_DIARY.md): design decisions.
For previous design decisions and changes, see the [development diary](DEVELOPMENT_DIARY.md) and Git commit history.

---

[Project overview](../README.md) | [日本語](DOCS.ja.md) | [Development diary](DEVELOPMENT_DIARY.md)
