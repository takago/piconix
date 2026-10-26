# PicoNIX

[日本語](README-JP.md) | [Documentation](#documentation)

PicoNIX is a **small UNIX-like operating system for the Raspberry Pi Pico (RP2040)**. It is not a Linux port. It is an experimental OS for studying scheduling, system calls, minimal files, and device I/O in a compact codebase.

## What it can do

- Preemptive task switching using SysTick and PendSV
- An SVC boundary between the kernel and RAM applications
- Commands including `ps`, `kill`, `blink`, `omikuji`, and `python`
- Simple background execution with `&` and foreground interruption via `Ctrl+C`
- Virtual files under `/dev` and `/sys/proc`, and volatile files under `/tmp`
- Basic stdin/stdout redirection (`<`, `>`, `>>`)
- A tiny Python-like interpreter supporting integer expressions and GPIO

## Quick start

Build on Linux with Pico SDK 2.1.1 and ARM GCC. Adjust the paths to your environment:

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

Copy `build/piconix.uf2` to a Pico in BOOTSEL mode and connect a USB CDC serial terminal. See [Setup](SETUP-EN.md).

```text
piconix# help
piconix# ps
piconix# blink &
piconix# cat /sys/proc/uptime
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# python
```

## Design and limitations

PicoNIX uses fixed 64 KiB RAM regions for the kernel and for Shell, Blink, and Omikuji/Python. Application images are embedded in flash and run from their designated RAM slots. **Omikuji and Python share a slot and cannot run concurrently.**

PicoNIX is not a fully UNIX-compatible OS. It has no generic signal delivery, signal handlers, pipes, per-process memory protection, or dynamic ELF loading. `Ctrl+C` and `kill` terminate tasks directly in the kernel; they do not send UNIX signals. `/tmp` is lost on reboot.

## Documentation

| Goal | Guide |
|---|---|
| Prepare, build, and flash | [Setup](SETUP-EN.md) |
| Understand RAM, tasks, and syscalls | [Internals](INTERNAL-EN.md) |
| Write a C RAM application | [Application programming](PROGRAMMING-EN.md) |
| Use the `python` command | [Python-like interpreter](PYTHON-EN.md) |

## License

GPL-3.0-or-later. See `LICENSE`.
