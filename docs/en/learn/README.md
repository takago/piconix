# PicoNIX UNIX Textbook — Learning Path

Follow the sequence **use → observe → read source → extend** from beginner (Part A) to advanced (Part C). Use the Next link at the end of each chapter to continue.

**Need a particular operation or feature?** Use the [task index](../TASKS.md).

## Three Parts

- **[Part A — Beginner: Use UNIX](A/README.md)**
- **[Part B — Intermediate: Observe UNIX](B/README.md)**
- **[Part C — Advanced: Read and extend UNIX](C/README.md)**

## Complete chapter map

### Part A

- **[A1 Entering PicoNIX and UNIX](A/A1.md)** 
  - [A1.1 What does it mean to use UNIX?](A/A1.md#a11-what-does-it-mean-to-use-unix)
  - [A1.2 Connect to the console](A/A1.md#a12-connect-to-the-console)
  - [A1.3 Explore the namespace](A/A1.md#a13-explore-the-namespace)
  - [A1.4 Practice and check](A/A1.md#a14-practice-and-check)
- **[A2 Files and streams](A/A2.md)** 
  - [A2.1 Create and read a file](A/A2.md#a21-create-and-read-a-file)
  - [A2.2 Standard input and output](A/A2.md#a22-standard-input-and-output)
  - [A2.3 Connect programs with a pipe](A/A2.md#a23-connect-programs-with-a-pipe)
  - [A2.4 Practice and check](A/A2.md#a24-practice-and-check)
- **[A3 Programs and processes](A/A3.md)** 
  - [A3.1 Available does not mean running](A/A3.md#a31-available-does-not-mean-running)
  - [A3.2 Run a background task](A/A3.md#a32-run-a-background-task)
  - [A3.3 Kernel resource management](A/A3.md#a33-kernel-resource-management)
  - [A3.4 Practice and check](A/A3.md#a34-practice-and-check)
- **[A4 Beginner capstone: a small UNIX experiment](A/A4.md)** 
  - [A4.1 Design a simple observation](A/A4.md#a41-design-a-simple-observation)
  - [A4.2 Record results](A/A4.md#a42-record-results)
  - [A4.3 Three UNIX viewpoints](A/A4.md#a43-three-unix-viewpoints)
  - [A4.4 Check your progress](A/A4.md#a44-check-your-progress)

### Part B

- **[B1 File descriptors and VFS](B/B1.md)** 
  - [B1.1 What is an FD?](B/B1.md#b11-what-is-an-fd)
  - [B1.2 Observe FDs](B/B1.md#b12-observe-fds)
  - [B1.3 A common namespace with VFS](B/B1.md#b13-a-common-namespace-with-vfs)
  - [B1.4 Practice and check](B/B1.md#b14-practice-and-check)
- **[B2 TTY and terminal I/O](B/B2.md)** 
  - [B2.1 Inspect the terminal](B/B2.md#b21-inspect-the-terminal)
  - [B2.2 Line input vs character input](B/B2.md#b22-line-input-vs-character-input)
  - [B2.3 The kernel and the application](B/B2.md#b23-the-kernel-and-the-application)
  - [B2.4 Practice and check](B/B2.md#b24-practice-and-check)
- **[B3 Flash, RAM, and tasks](B/B3.md)** 
  - [B3.1 What stays in Flash?](B/B3.md#b31-what-stays-in-flash)
  - [B3.2 Inspect memory](B/B3.md#b32-inspect-memory)
  - [B3.3 Why fixed slots?](B/B3.md#b33-why-fixed-slots)
  - [B3.4 Practice and check](B/B3.md#b34-practice-and-check)
- **[B4 Observe system calls with trace](B/B4.md)** 
  - [B4.1 Form a hypothesis](B/B4.md#b41-form-a-hypothesis)
  - [B4.2 Run trace](B/B4.md#b42-run-trace)
  - [B4.3 Interpret the boundary](B/B4.md#b43-interpret-the-boundary)
  - [B4.4 Practice and check](B/B4.md#b44-practice-and-check)
- **[B5 Devices as files](B/B5.md)** 
  - [B5.1 List devices safely](B/B5.md#b51-list-devices-safely)
  - [B5.2 Read/write and ioctl](B/B5.md#b52-readwrite-and-ioctl)
  - [B5.3 Verify wiring before hardware tests](B/B5.md#b53-verify-wiring-before-hardware-tests)
  - [B5.4 Practice and check](B/B5.md#b54-practice-and-check)
- **[B6 Optional lab: Pico W networking](B/B6.md)** (optional / Pico W)
  - [B6.1 Hardware and security](B/B6.md#b61-hardware-and-security)
  - [B6.2 Inspect Pico W](B/B6.md#b62-inspect-pico-w)
  - [B6.3 Access from a Linux PC](B/B6.md#b63-access-from-a-linux-pc)
  - [B6.4 Practice and check](B/B6.md#b64-practice-and-check)

### Part C

- **[C1 Start with a user program](C/C1.md)** 
  - [C1.1 Read an application](C/C1.md#c11-read-an-application)
  - [C1.2 Write a small command](C/C1.md#c12-write-a-small-command)
  - [C1.3 Build and flash](C/C1.md#c13-build-and-flash)
  - [C1.4 Practice and check](C/C1.md#c14-practice-and-check)
- **[C2 Boot, tasks, and scheduling](C/C2.md)** 
  - [C2.1 From main to sh](C/C2.md#c21-from-main-to-sh)
  - [C2.2 Prepare and switch tasks](C/C2.md#c22-prepare-and-switch-tasks)
  - [C2.3 RAM slots and registers](C/C2.md#c23-ram-slots-and-registers)
  - [C2.4 Reading assignment](C/C2.md#c24-reading-assignment)
- **[C3 Follow SVC, FDs, and VFS in source](C/C3.md)** 
  - [C3.1 From user code to SVC](C/C3.md#c31-from-user-code-to-svc)
  - [C3.2 From syscall to I/O object](C/C3.md#c32-from-syscall-to-io-object)
  - [C3.3 Pipes and waiting](C/C3.md#c33-pipes-and-waiting)
  - [C3.4 Reading assignment](C/C3.md#c34-reading-assignment)
- **[C4 Drivers: from VFS to hardware](C/C4.md)** 
  - [C4.1 Begin with VFS](C/C4.md#c41-begin-with-vfs)
  - [C4.2 Read/write and ioctl](C/C4.md#c42-readwrite-and-ioctl)
  - [C4.3 Check before editing](C/C4.md#c43-check-before-editing)
  - [C4.4 Reading assignment](C/C4.md#c44-reading-assignment)
- **[C5 Optional source tour: Pico W networking](C/C5.md)** (optional / Pico W)
  - [C5.1 Where this fits](C/C5.md#c51-where-this-fits)
  - [C5.2 Two network layers](C/C5.md#c52-two-network-layers)
  - [C5.3 Kernel vs user-space services](C/C5.md#c53-kernel-vs-user-space-services)
  - [C5.4 Reading assignment](C/C5.md#c54-reading-assignment)
- **[C6 Capstone: extend the OS safely](C/C6.md)** 
  - [C6.1 Design before editing](C/C6.md#c61-design-before-editing)
  - [C6.2 Compare extension paths](C/C6.md#c62-compare-extension-paths)
  - [C6.3 Modify and test incrementally](C/C6.md#c63-modify-and-test-incrementally)
  - [C6.4 Final deliverables](C/C6.md#c64-final-deliverables)

## Conventions

- `piconix#` denotes the PicoNIX prompt, not text to type. Host commands are explicitly marked as Linux PC commands.
- Required lessons work with USB and a regular Pico. Wi-Fi chapters are optional and require Pico W.
- `/tmp` and `/etc` contain RAM files that disappear on reboot; recreate exercise files as needed.
- Record **command → observation → explanation**. PIDs, times, and addresses may change.
- Read the chapters independently; consult [references](../reference/README.md) and [supplementary guides](../guides/README.md) only for deeper details.
- PicoNIX is a teaching OS, not a feature-for-feature implementation of Linux or POSIX.

## Start here

**[A1: Entering PicoNIX and UNIX](A/A1.md)**

---

**Related links:** [Documentation directory](../../DOCS.md) | [日本語](../../ja/learn/README.md)
