# PicoNIX

[日本語](README-JP.md) | [Documentation](#documentation)

PicoNIX is a **small UNIX-like operating system for the Raspberry Pi Pico (RP2040)**. It is not a Linux port. It is an experimental OS for studying scheduling, system calls, minimal files, and device I/O in a compact codebase.

## What it can do

- Preemptive task switching using SysTick and PendSV
- An SVC boundary between the kernel and RAM applications
- Commands including `ps`, `kill`, `sh`, `getty`, `blink`, `omikuji`, and `python`
- Simple background execution with `&` and foreground interruption via `Ctrl+C`
- Virtual files under `/dev` and `/proc`, and volatile files under `/tmp`
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
piconix# cat /proc/uptime
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# python
```

## Two-terminal operation

Run `getty /dev/uart1 sh &` in the USB shell to start an independent UART1 shell. Its children inherit standard I/O on UART1. Use `getty /dev/uart1 python &` for a Python REPL. Configure UART1 for 115200 baud, 8N1 and local echo. Starting another `getty` on an occupied terminal is rejected. Hardware behavior remains unverified.

## Design and limitations

PicoNIX has a 64 KiB kernel region and six 32 KiB application slots. PIC application images are embedded in flash and loaded into a free slot on launch. Up to six application tasks can run concurrently.

PicoNIX is not a fully UNIX-compatible OS. It has no generic signal delivery, signal handlers, pipes, per-process memory protection, or dynamic ELF loading. `Ctrl+C` and `kill` terminate tasks directly in the kernel; they do not send UNIX signals. `/tmp` is lost on reboot.

## Documentation

| Goal | Guide |
|---|---|
| Prepare, build, and flash | [Setup](SETUP-EN.md) |
| Understand RAM, tasks, and syscalls | [Internals](INTERNAL-EN.md) |
| Write a C RAM application | [Application programming](PROGRAMMING-EN.md) |
| Understand UART1, `getty`, Ctrl+C/Ctrl+D | [UART and terminals](UART-EN.md) |
| Use the `python` command | [Python-like interpreter](PYTHON-EN.md) |

## License

GPL-3.0-or-later. See `LICENSE`.

Interactive Python over UART1: `python < /dev/uart1 > /dev/uart1 &` (hardware validation pending).

Use `tty` to display the current controlling terminal. The `TTY` column of `ps` and `/proc/tasks` shows each task’s terminal (`console` or `uart1`). Ordinary I/O redirection does not change terminal affiliation.

When UART1 is attached by `getty`, Ctrl+D signals end-of-file (EOF). With no terminal owner, UART1 remains a raw byte stream and delivers `0x04` unchanged. In the UART1 shell and Python REPL, Ctrl+D does not terminate the session. Use `logout` or `exit` for the shell and `exit()` for Python.
