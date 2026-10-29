# PicoNIX

[日本語](../ja/README.md) | [Documentation](#documentation)

PicoNIX is a **small UNIX-like operating system for the Raspberry Pi Pico (RP2040)**. It is not a Linux port. It is an experimental OS for studying scheduling, system calls, minimal files, and device I/O in a compact codebase.

## What it can do

- Preemptive task switching using SysTick and PendSV
- An SVC boundary between the kernel and RAM applications
- Commands including `ps`, `kill`, `sh`, `getty`, `blink`, `omikuji`, `python`, `hello`, and `hexdump`
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

Copy `build/piconix.uf2` to a Pico in BOOTSEL mode and connect a USB CDC serial terminal. See [Setup](SETUP.md).

```text
piconix# help
piconix# help gpio
piconix# ps
piconix# blink &
piconix# cat /proc/uptime
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# python
```

## Two-terminal operation

Run `getty /dev/uart1 sh &` in the USB shell to start an independent UART1 shell. Its children inherit standard I/O on UART1. Use `getty /dev/uart1 python &` for a Python REPL. Configure UART1 for 115200 baud, 8N1 and local echo. Starting another `getty` on an occupied terminal is rejected. 

## Hardware and wiring

The default setup uses a USB CDC console and an 8×8 LED matrix. UART1 uses GP20 (TX) / GP21 (RX), PWM uses GP18/19, and ADC uses GP26–28. Wiring and electrical precautions are in the [Device guide](DEVICES.md).

## Design and limitations

PicoNIX has a 96 KiB kernel region and five 32 KiB application slots. PIC application images are embedded in flash and loaded into a free slot on launch. Up to five application tasks can run concurrently.

PicoNIX is not a fully UNIX-compatible OS. It has no generic signal delivery, signal handlers, pipes, per-process memory protection, or dynamic ELF loading. `Ctrl+C` and `kill` terminate tasks directly in the kernel; they do not send UNIX signals. `/tmp` is lost on reboot.

[Device guide](DEVICES.md) — GPIO, ADC, PWM, and virtual devices.

## Documentation

| Goal | Guide |
|---|---|
| Prepare, build, and flash | [Setup](SETUP.md) |
| Understand RAM, tasks, and syscalls | [Internals](INTERNAL.md) |
| Write a C RAM application | [Application programming](PROGRAMMING.md) |
| Understand UART1, `getty`, Ctrl+C/Ctrl+D | [UART and terminals](UART.md) |
| Use the `python` command | [Python-like interpreter](PYTHON.md) |
| Configure TTY input and play `mb` | [Terminal settings and interactive input](TTY.md) |
| Use the LED framebuffer | [Device guide (framebuffer)](DEVICES.md) |
| Inspect kernel tracing | [Trace](TRACE.md) |


## License

GPL-3.0-or-later. See `LICENSE`.

- [Tiny Thumb assembler](ASM.md) — `as` / `./a.out`

`/tmp` supports four files of up to 1 KiB each. `ls -l` shows the node type and file size where available.
