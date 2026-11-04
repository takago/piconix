# PicoNIX UNIX Textbook — Learning Path

**Document type: TEXTBOOK CONTENTS** | [Documentation directory](../../DOCS.md) | [日本語](../../ja/learn/README.md)

A linear textbook built around **use → observe → read source → extend**. **Only numbered chapters and sections (such as `A1.1`) belong to the core textbook.**

**Identify your location:** Each textbook page has a “LEARNING” label and a chapter number. When reading references, use your browser Back action or the textbook chapter navigation to resume. Reference pages do not carry textbook Next/Previous links.

**Need a particular operation or feature?** Use the [task index](../TASKS.md).

## Three Parts

- **[Part A — Beginner: Use UNIX](A/README.md)**
- **[Part B — Intermediate: Observe UNIX](B/README.md)**
- **[Part C — Advanced: Read and extend UNIX](C/README.md)**

## Complete chapter map

### Part A

- **[A1 Entering PicoNIX and UNIX](A/A1.md)** 
  - A1.1 What does it mean to use UNIX?
  - A1.2 Connect to the console
  - A1.3 Explore the namespace
  - A1.4 Practice and check
- **[A2 Files and streams](A/A2.md)** 
  - A2.1 Create and read a file
  - A2.2 Standard input and output
  - A2.3 Connect programs with a pipe
  - A2.4 Practice and check
- **[A3 Programs and processes](A/A3.md)** 
  - A3.1 Available does not mean running
  - A3.2 Run a background task
  - A3.3 Kernel resource management
  - A3.4 Practice and check
- **[A4 Beginner capstone: a small UNIX experiment](A/A4.md)** 
  - A4.1 Design a simple observation
  - A4.2 Record results
  - A4.3 Three UNIX viewpoints
  - A4.4 Check your progress

### Part B

- **[B1 File descriptors and VFS](B/B1.md)** 
  - B1.1 What is an FD?
  - B1.2 Observe FDs
  - B1.3 A common namespace with VFS
  - B1.4 Practice and check
- **[B2 TTY and terminal I/O](B/B2.md)** 
  - B2.1 Inspect the terminal
  - B2.2 Line input vs character input
  - B2.3 The kernel and the application
  - B2.4 Practice and check
- **[B3 Flash, RAM, and tasks](B/B3.md)** 
  - B3.1 What stays in Flash?
  - B3.2 Inspect memory
  - B3.3 Why fixed slots?
  - B3.4 Practice and check
- **[B4 Observe system calls with trace](B/B4.md)** 
  - B4.1 Form a hypothesis
  - B4.2 Run trace
  - B4.3 Interpret the boundary
  - B4.4 Practice and check
- **[B5 Devices as files](B/B5.md)** 
  - B5.1 List devices safely
  - B5.2 Read/write and ioctl
  - B5.3 Verify wiring before hardware tests
  - B5.4 Practice and check
- **[B6 Optional lab: Pico W networking](B/B6.md)** (optional / Pico W)
  - B6.1 Hardware and security
  - B6.2 Inspect Pico W
  - B6.3 Access from a Linux PC
  - B6.4 Practice and check

### Part C

- **[C1 Start with a user program](C/C1.md)** 
  - C1.1 Read an application
  - C1.2 Write a small command
  - C1.3 Build and flash
  - C1.4 Practice and check
- **[C2 Boot, tasks, and scheduling](C/C2.md)** 
  - C2.1 From main to sh
  - C2.2 Prepare and switch tasks
  - C2.3 RAM slots and registers
  - C2.4 Reading assignment
- **[C3 Follow SVC, FDs, and VFS in source](C/C3.md)** 
  - C3.1 From user code to SVC
  - C3.2 From syscall to I/O object
  - C3.3 Pipes and waiting
  - C3.4 Reading assignment
- **[C4 Drivers: from VFS to hardware](C/C4.md)** 
  - C4.1 Begin with VFS
  - C4.2 Read/write and ioctl
  - C4.3 Check before editing
  - C4.4 Reading assignment
- **[C5 Optional source tour: Pico W networking](C/C5.md)** (optional / Pico W)
  - C5.1 Where this fits
  - C5.2 Two network layers
  - C5.3 Kernel vs user-space services
  - C5.4 Reading assignment
- **[C6 Capstone: extend the OS safely](C/C6.md)** 
  - C6.1 Design before editing
  - C6.2 Compare extension paths
  - C6.3 Modify and test incrementally
  - C6.4 Final deliverables

## Conventions

- `piconix#` denotes the PicoNIX prompt, not text to type. Host commands are explicitly marked as Linux PC commands.
- Required lessons work with USB and a regular Pico. Wi-Fi chapters are optional and require Pico W.
- `/tmp` and `/etc` contain RAM files that disappear on reboot; recreate exercise files as needed.
- Record **command → observation → explanation**. PIDs, times, and addresses may change.
- Read the chapters independently; consult [references](../reference/README.md) and [supplementary guides](../guides/README.md) only for deeper details.
- PicoNIX is a teaching OS, not a feature-for-feature implementation of Linux or POSIX.

## Start here

**[A1: Entering PicoNIX and UNIX](A/A1.md)**
