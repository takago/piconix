# PicoNIX Internal Architecture (Stage 30)

[日本語](INTERNAL-JP.md) | [English](INTERNAL-EN.md)

This document describes the implementation and intentional simplifications. It is not a specification of full UNIX compatibility.

## 1. Overview and memory map

PicoNIX combines a Pico SDK-based kernel image with standalone fixed-address RAM applications embedded in flash.

| Region | Base address | Size | Role |
|---|---|---|---|
| Kernel | `0x20000000` | 64 KiB | Pico SDK support, kernel and scheduler |
| Slot 1 | `0x20010000` | 64 KiB | Shell |
| Slot 2 | `0x20020000` | 64 KiB | Blink |
| Slot 3 | `0x20030000` | 64 KiB | Omikuji **or** Python |

The top 4 KiB of each application slot is reserved for its stack. Applications use statically assigned addresses; they are not loaded from arbitrary filenames and are not relocated.

## 2. Boot sequence and linking

The Pico SDK provides startup initialization. The kernel initializes devices, registers built-in program images, starts the shell, and schedules tasks. Flash-embedded application images are copied into their fixed RAM slots for execution. The custom linker configuration restricts kernel RAM occupancy to 64 KiB; `tools/` checks flash and RAM placement.

## 3. Scheduling and task lifecycle

SysTick drives periodic scheduling and PendSV saves/restores task contexts. Tasks use SVC for kernel services. The shell launches known applications through `spawn`, optionally waits for foreground completion, and may manage a background child. Termination status is recorded for `waitpid`. The wait primitive in the kernel is nonblocking; user-side wrappers can supply blocking waits.

## 4. Foreground versus background

The console tracks the foreground task. `Ctrl+C` is interpreted in the kernel's input path. A background task should not consume foreground console input. The Shell recognizes a final `&` before handling redirection and periodically reaps completed children without displaying completion messages.

Omikuji has a special no-console-input behavior when launched in the background without an explicit countdown. The Python interpreter supports background **script** execution, but not a background interactive REPL.

## 5. System-call boundary

System calls are performed through `svc #0`. The kernel validates pointers and descriptor arguments where required. Keep syscall numbers stable when modifying applications; standalone RAM images do not link to kernel C symbols.

`dup2` duplicates descriptor values and positions, not an underlying POSIX open-file object; offsets are not shared. Descriptor slots 0, 1, and 2 represent standard input, output, and error. Descriptor inheritance is intentionally minimal.

## 6. Devices and virtual files

The device registry includes `/dev/console`, `/dev/uart1`, and `/dev/rand`. GPIO is accessed through `/dev/gpio/N` and associated `.dir` and `.pull` nodes. The virtual `/sys/proc` tree provides runtime information such as tasks, uptime, ticks, memory, devices, interrupts, and context-switch counts.

Files under `/tmp` are limited, volatile RAM-backed regular files. The Shell resolves relative paths using its current directory; the kernel does not implement general-purpose filesystem pathname traversal. Opening a device for redirection does not require `O_CREAT` or `O_TRUNC`; those flags belong to regular files.

## 7. Input/output redirection

Shell redirection uses `open` and `dup2` on per-task descriptor tables and restores the original standard descriptors after command execution. File append and truncation are simplified. Pipes, quoted paths and standard-error redirection are not implemented.

## 8. Boundaries and future changes

There is no general executable loader, MMU, POSIX-compatible file-offset sharing, advanced terminal driver, or simultaneous execution of Omikuji and Python. Preserve the compact source organization, 64 KiB kernel RAM constraint, application ABI and existing functionality before adding abstractions.

## 9. Main files

`src/kernel.c`: kernel and devices; `src/context.S`: context switching; `src/main.c`: startup; `include/piconix.h`: kernel API; `ram_apps/*.c`: freestanding applications; `ram_apps/slot.ld`: slot linking; `tools/`: build and audits.

See [Programming](PROGRAMMING-EN.md) and [Setup](SETUP-EN.md).
