# PicoNIX Documentation

Documentation is grouped into **setup, technical references, and developer references**. Each document describes the current implementation, its limits, and relevant usage or implementation examples. Historical behavior and design changes belong in the [development diary](DEVELOPMENT_DIARY.md).

**Prerequisites:** Kernel and application development references assume familiarity with C, basic UNIX/Linux operations, processes, file descriptors, and system calls. See [Differences from Linux](reference/DIFFERENCES.md) for PicoNIX-specific behavior.

## Setup

[Setup and flashing](setup/SETUP.md): toolchains, builds, Pico/Pico W flashing, and terminal connections.

## Technical references

[Reference index](reference/README.md): APIs, system calls, file descriptors, memory layout, devices, networking, and resource limits.

By topic: [Commands](reference/commands/README.md) | [Hardware and wiring](reference/hardware/README.md) | [Devices and I/O](reference/devices/README.md) | [Networking (Pico W)](reference/network/README.md).

## Developer references

[Development index](development/README.md): C, Python and assembler interfaces and examples, kernel internals, and extension procedures for syscalls, drivers, and networking.

## Task index

[Find documentation by task](TASKS.md): find a command, specification, or source-level interface by purpose.

## Source index and development records

- [Function index](development/FUNCTIONS.md): locating source functions.
- [Development diary](DEVELOPMENT_DIARY.md): chronological project record (not the current specification).

---

[Project overview](../../README.md) | [日本語](../ja/README.md) | [Development diary](DEVELOPMENT_DIARY.md)
