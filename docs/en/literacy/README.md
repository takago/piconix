# PicoNIX

**First time here?** Follow the [guided learning path](LEARNING-PATH.md) for hands-on exercises before the detailed reference.

**File browser:** [fd](FD.md) supports directory navigation, live viewing, and `/tmp` editing.

**Hands-on themes:** [System monitoring](MONITORING.md) (Pico / Pico W) and [Networking](NETWORKING.md) (Pico W) are separate labs.

[日本語](../../ja/literacy/README.md) | [Documentation guide](../../DOCS.md)

PicoNIX is a **small UNIX-like operating system for the Raspberry Pi Pico (RP2040)**. It is not a Linux port. It is an experimental OS for studying scheduling, system calls, minimal files, and device I/O in a compact codebase.

PicoNIX provides six fixed 24 KiB application slots.

## What it can do

- **User-process TCP/UDP networking on Pico W:** `socket()`, `send()`, `recv()`, `sendto()`, and `recvfrom()` with `nc`, `httpd`, `tcp_echod`, and `udp_echod`. See [Socket API and network applications](../development/apps/c/NETWORK-PROGRAMMING.md).
- **Inspect network packets:** `tcpdump` and `ping` on Pico W; use the [networking lab](NETWORKING.md).
- Preemptive task switching using SysTick and PendSV
- An SVC boundary between the kernel and RAM applications
- Commands including `ps`, `kill`, `sh`, `getty`, `blink`, `omikuji`, `python`, `hello`, `hexdump`, `dd`, `vi`, and `edit`
- Simple background execution with `&` and foreground interruption via `Ctrl+C`
- Virtual files under `/dev` and `/proc`, and volatile files under `/tmp`
- Basic stdin/stdout redirection (`<`, `>`, `>>`) and pipelines (`|`) of up to three commands
- Binary copying with `dd`, and text editing with `vi` and `edit` for RAM files up to 2 KiB
- A tiny Python-like interpreter supporting integer expressions and GPIO

`nc -u -N` exits on stdin EOF for UDP, and Ctrl+C interrupts foreground commands and pipelines. See the [nc guide](../reference/devices/NC.md).

## Quick start

Build on Linux with Pico SDK 2.1.1 and ARM GCC. Adjust the paths to your environment:

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

For Pico W, replace `-DPICO_BOARD=pico` with `-DPICO_BOARD=pico_w`. Copy `build-pico/piconix.uf2` (Pico) or `build-pico_w/piconix.uf2` (Pico W) in BOOTSEL mode and connect a USB CDC serial terminal. See [Setup](../setup/SETUP.md).

```text
piconix# help
piconix# help gpio
piconix# ps
piconix# blink &
piconix# cat /proc/uptime
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# cat -n /tmp/hello.txt
piconix# python
```

## Two-terminal operation

Run `getty /dev/ttyS1 sh &` in the USB shell to start an independent UART1 shell. Its children inherit standard I/O on UART1. Use `getty /dev/ttyS1 python &` for a Python REPL. Configure UART1 for 115200 baud, 8N1 and local echo. Starting another `getty` on an occupied terminal is rejected. 

## Hardware and wiring

The default setup uses a USB CDC console and an 8×8 LED matrix. UART1 uses GP20 (TX) / GP21 (RX), PWM uses GP18/19, I²C1 uses GP26 (SDA) / GP27 (SCL), and ADC2 uses GP28. Wiring and electrical precautions are in the [Device guide](../reference/devices/DEVICES.md).

## Design and limitations

PicoNIX has a 56 KiB kernel region and six 24 KiB application slots (4 KiB reserved for the stack in each slot). PIC application images are embedded in flash and loaded into a free slot on launch. Up to six application tasks can run concurrently.

`cat -n` numbers displayed lines, including empty lines; it does not modify the file.

PicoNIX is not a fully UNIX-compatible OS. It has no generic signal delivery, signal handlers, per-process memory protection, or dynamic ELF loading. `Ctrl+C` and `kill` terminate tasks directly in the kernel; they do not send UNIX signals. `/tmp` and `/etc` are RAM-backed and lost on reboot; they share a pool of up to eleven 2 KiB files.

## Where to go next

For a guided reading order, see the [documentation index](../../DOCS.md). For Pico W networking, see the [network guide](../reference/devices/NETWORK.md).


| Goal | Guide |
|---|---|
| Follow the development history and design decisions | [Development diary](../../DEVELOPMENT_DIARY.md) |
| Prepare, build, and flash | [Setup](../setup/SETUP.md) |
| Understand RAM and task management | [Internals](../reference/INTERNAL.md) |
| Check syscall numbers and the calling convention | [System calls](../reference/SYSCALLS.md) |
| Try assembly with `as` | [Tiny Thumb assembler](../development/apps/asm/ASM.md) |
| Learn about pipes, `grep`, and descriptors | [Pipes and standard I/O](PIPE.md) |
| Write a C RAM application | [Application programming](../development/apps/c/PROGRAMMING.md) |
| Build TCP/UDP clients and servers | [Sockets and network applications](../development/apps/c/NETWORK-PROGRAMMING.md) |
| Work with GPIO, ADC, and PWM | [Device guide](../reference/devices/DEVICES.md) |
| Understand UART1, `getty`, Ctrl+C/Ctrl+D | [UART and terminals](../reference/devices/UART.md) |
| Use the `python` command | [Python-like interpreter](../development/apps/python/PYTHON.md) |
| Copy binary data with `dd` | [dd guide](DD.md) |
| Edit files with `vi` or `edit` | [vi editor](VI.md) / [Text editor](EDIT.md) |
| Connect I²C devices and use `i2cctl` or `bh1750` | [I²C1 guide](../reference/devices/I2C.md) |
| Configure TTY input and play `mb` | [Terminal settings and interactive input](TTY.md) |
| Use the LED framebuffer | [Device guide (framebuffer)](../reference/devices/DEVICES.md) |
| Inspect kernel tracing | [Trace](TRACE.md) |
| Understand filesystem nodes | [Filesystem](../reference/FILESYSTEM.md) |


## License

GPL-3.0-or-later. See `LICENSE`.


[Documentation index](../../DOCS.md) | [日本語](../../ja/literacy/README.md)
