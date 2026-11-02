# PicoNIX

[日本語](../ja/README.md) | [Documentation guide](DOCS.md)

PicoNIX is a **small UNIX-like operating system for the Raspberry Pi Pico (RP2040)**. It is not a Linux port. It is an experimental OS for studying scheduling, system calls, minimal files, and device I/O in a compact codebase.

PicoNIX provides six fixed 24 KiB application slots.

## What it can do

- **User-process TCP/UDP networking on Pico W:** `socket()`, `send()`, `recv()`, `sendto()`, and `recvfrom()` with `nc`, `httpd`, `tcp_echod`, and `udp_echod`. See [Socket API and network applications](NETWORK-PROGRAMMING.md).
- Preemptive task switching using SysTick and PendSV
- An SVC boundary between the kernel and RAM applications
- Commands including `ps`, `kill`, `sh`, `getty`, `blink`, `omikuji`, `python`, `hello`, and `hexdump`
- Simple background execution with `&` and foreground interruption via `Ctrl+C`
- Virtual files under `/dev` and `/proc`, and volatile files under `/tmp`
- Basic stdin/stdout redirection (`<`, `>`, `>>`) and pipelines (`|`) of up to three commands
- A tiny Python-like interpreter supporting integer expressions and GPIO

`nc -u -N` exits on stdin EOF for UDP, and Ctrl+C interrupts foreground commands and pipelines. See the [nc guide](NC.md).

## Quick start

Build on Linux with Pico SDK 2.1.1 and ARM GCC. Adjust the paths to your environment:

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

Copy `build-pico/piconix.uf2` (Pico) or `build-pico_w/piconix.uf2` (Pico W) in BOOTSEL mode and connect a USB CDC serial terminal. See [Setup](SETUP.md).

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

Run `getty /dev/ttyS1 sh &` in the USB shell to start an independent UART1 shell. Its children inherit standard I/O on UART1. Use `getty /dev/ttyS1 python &` for a Python REPL. Configure UART1 for 115200 baud, 8N1 and local echo. Starting another `getty` on an occupied terminal is rejected. 

## Hardware and wiring

The default setup uses a USB CDC console and an 8×8 LED matrix. UART1 uses GP20 (TX) / GP21 (RX), PWM uses GP18/19, I²C1 uses GP26 (SDA) / GP27 (SCL), and ADC2 uses GP28. Wiring and electrical precautions are in the [Device guide](DEVICES.md).

## Design and limitations

PicoNIX has a 64 KiB kernel region and six 24 KiB application slots (4 KiB reserved for the stack in each slot). PIC application images are embedded in flash and loaded into a free slot on launch. Up to six application tasks can run concurrently.

PicoNIX is not a fully UNIX-compatible OS. It has no generic signal delivery, signal handlers, per-process memory protection, or dynamic ELF loading. `Ctrl+C` and `kill` terminate tasks directly in the kernel; they do not send UNIX signals. `/tmp` is lost on reboot.

## Where to go next

For a guided reading order, see the [documentation index](DOCS.md). For Pico W networking, see the [network guide](NETWORK.md).


| Goal | Guide |
|---|---|
| Follow the development history and design decisions | [Development diary](DEVELOPMENT_DIARY.md) |
| Prepare, build, and flash | [Setup](SETUP.md) |
| Understand RAM and task management | [Internals](INTERNAL.md) |
| Check syscall numbers and the calling convention | [System calls](SYSCALLS.md) |
| Try assembly with `as` | [Tiny Thumb assembler](ASM.md) |
| Learn about pipes, `grep`, and descriptors | [Pipes and standard I/O](PIPE.md) |
| Write a C RAM application | [Application programming](PROGRAMMING.md) |
| Build TCP/UDP clients and servers | [Sockets and network applications](NETWORK-PROGRAMMING.md) |
| Work with GPIO, ADC, and PWM | [Device guide](DEVICES.md) |
| Understand UART1, `getty`, Ctrl+C/Ctrl+D | [UART and terminals](UART.md) |
| Use the `python` command | [Python-like interpreter](PYTHON.md) |
| Connect I²C devices and use `i2cctl` or `bh1750` | [I²C1 guide](I2C.md) |
| Configure TTY input and play `mb` | [Terminal settings and interactive input](TTY.md) |
| Use the LED framebuffer | [Device guide (framebuffer)](DEVICES.md) |
| Inspect kernel tracing | [Trace](TRACE.md) |
| Understand filesystem nodes | [Filesystem](FILESYSTEM.md) |


## License

GPL-3.0-or-later. See `LICENSE`.

See [dd guide](DD.md) and [vi editor](VI.md).
