# PicoNIX

[日本語](README-JP.md) | [Documentation](#documentation)

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

Run `getty /dev/uart1 sh &` in the USB shell to start an independent UART1 shell. Its children inherit standard I/O on UART1. Use `getty /dev/uart1 python &` for a Python REPL. Configure UART1 for 115200 baud, 8N1 and local echo. Starting another `getty` on an occupied terminal is rejected. 

## Expected external devices and wiring

The following is the intended wiring example for the standard PicoNIX configuration (USB CDC console and 8×8 LED framebuffer). **The OS does not automatically identify attached components.**

| GPIO | External connection | Purpose / notes |
|---|---|---|
| GP0–GP7 | 8×8 LED matrix columns (cathodes) | Columns 0–7; active LOW |
| GP8–GP15 | 8×8 LED matrix rows (anodes) | Rows 0–7; active HIGH |
| GP16 | Push button | Between GPIO and GND; internal pull-up enabled |
| GP17 | Push button | Between GPIO and GND; internal pull-up enabled |
| GP18 | Passive buzzer or another PWM load | PWM slice 1, channel A; used by `kitesys` |
| GP19 | Optional PWM output | PWM slice 1, channel B; shares frequency with GP18 |
| GP20 | UART1 TX | Connect to the external device's RX |
| GP21 | UART1 RX | Connect to the external device's TX |
| GP26–GP28 | Optional analog signals | ADC0–ADC2; keep inputs within 0–3.3 V |

`kitesys` shifts digits using the GP16/17 buttons and sounds a buzzer on GP18. PWM is controlled through `/dev/pwm/1/freq`, `/dev/pwm/1/a` (GP18), and `/dev/pwm/1/b` (GP19). UART1 operates at 115200 baud, 8N1. **Share GND** with the connected serial device and use **3.3 V logic levels**; do not connect RS-232 voltage levels directly.

**The LED matrix needs current limiting and suitable drive circuitry.** A selected row may carry the combined current of several LEDs; use appropriate resistors and transistor drivers. This is not a direct-to-GPIO wiring schematic. See [Framebuffer](FB-JP.md) and [Device guide](DEVICES-EN.md). Use `pins` to inspect the current pin state.

## Design and limitations

PicoNIX has a 96 KiB kernel region and five 32 KiB application slots. PIC application images are embedded in flash and loaded into a free slot on launch. Up to five application tasks can run concurrently.

PicoNIX is not a fully UNIX-compatible OS. It has no generic signal delivery, signal handlers, pipes, per-process memory protection, or dynamic ELF loading. `Ctrl+C` and `kill` terminate tasks directly in the kernel; they do not send UNIX signals. `/tmp` is lost on reboot.

[Device guide](DEVICES-EN.md) — GPIO, ADC, PWM, and virtual devices.

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

`/dev/mem` is read-only. Try `hexdump /dev/mem 0x20018000 64` for RAM or `hexdump /dev/mem 0x10000000 64` for flash. Only the 256-KiB main SRAM and first 2 MiB of XIP flash are readable; peripheral registers are excluded.

### Kernel tracing (16 KiB)

`trace on` clears and starts the binary ring; `trace off` stops recording; `trace show` stops recording and prints the retained events. `/dev/trace` provides read-only, fixed 32-byte event records. Trace controls use device-specific `ioctl()` commands; decoding stays in the `trace.c` user application.

```text
trace on
hello
trace off
trace show
```

Events include task switches and SVC entry/exit. Timestamps use SysTick milliseconds since boot.

- `/bin`: read-only list of registered external commands. `spawn` supports `argv[]` with up to eight arguments and 128 bytes of strings.

Trace details: [`TRACE-EN.md`](TRACE-EN.md) / [`TRACE-JP.md`](TRACE-JP.md)


## External ps, cat, watch and interrupt counters

`ps` and `cat` are standalone PIC applications (`/bin/ps`, `/bin/cat`), no longer Shell builtins. `watch` is a standalone `/bin/watch` application.

```text
ps
cat /proc/tasks
watch ps
watch 1 ps
watch 1 cat /proc/interrupts
watch 2 cat /proc/memory
```

`watch [seconds] command [args...]` repeatedly executes an external program. The interval is 1–60 seconds, default 1 second. The terminal is cleared using ANSI escapes. `watch` and its child occupy one task slot each. `ls` and `echo` are external commands and can run under `watch`. Ctrl+C only terminates the terminal's foreground PID, not a process group; a child may remain briefly if interrupted while running. Use short-lived commands.

`SysTick` in `/proc/interrupts` counts actual 1-ms interrupts. `uart1` counts entries into the UART1 IRQ handler. UART0-console builds report UART0 IRQs as `console`. USB CDC builds omit `console` because USB IRQs are not counted. `PendSV` and `SVC` count exception handler entries.


## External shell commands and working directories

`pwd`, `ls`, `rm`, `echo`, `kill`, and `tty` are separate `/bin` PIC applications. The `pid` command was removed; `getpid()` remains available. Shell built-ins are `help`, `cd`, `exit`, `logout`, and `getty`.

The kernel maintains one working directory per task, inherited through `spawn()`. `chdir()` changes the calling task's directory; `getcwd()` returns it. `open()` and `unlink()` resolve relative paths. After `cd /tmp`, `pwd`, `ls`, `cat name`, and `rm name` use that directory.

`ls /bin` lists external applications, followed by the Shell built-ins under their own heading. `watch 1 ls /tmp` and `watch 1 pwd` are now supported.

### 1-ms tick and ADC

SysTick runs at 1 ms, with a normal 10-ms time slice. The `TICKx1K` trace event is emitted once a second. `cat /dev/adc/0` through `2` reads GPIO26–28 raw ADC values (0–4095). Use `watch 1 cat /dev/adc/0` to sample repeatedly.

Read the RP2040 on-chip temperature sensor with `cat /dev/adc/temp` (approximate Celsius, assuming 3.3 V ADC reference).

### PWM and reserved GPIO pins (2026-10-27)

GP18 and GP19 are dedicated PWM outputs, sharing a default 1 kHz frequency, with initial duties 50% and 0%. `/dev/pwm/1/freq` accepts 8–100000 Hz, and `/dev/pwm/1/a` and `/dev/pwm/1/b` accept 0–100 percent via decimal text reads/writes. GPIO listings and access exclude GP18, GP19 and ADC-reserved GP26–28.


### Pin configuration and drivers (2026-10)

Edit `GPn_MODE` in `src/pins.h` and rebuild to change exposed `/dev/gpio`, `/dev/adc`, and `/dev/pwm` nodes and hardware initialization. Hardware drivers are in `src/gpio.c`, `src/adc.c`, and `src/pwm.c`; descriptor management remains in `src/kernel.c`. PWM defaults to 1 kHz and 0% duty on both GP18 and GP19. GP0/1 are GPIO in the default USB CDC build, while GP4/5 are reserved for UART1. For UART0 console builds, set GP0/1 to `PIN_UART`.

### Pinout command

Run `pins` to display the Raspberry Pi Pico 40-pin header as an ASCII diagram (USB connector at top), with live GPIO direction/level, framebuffer ownership, UART, ADC, and PWM duty/frequency from existing GPIO/PWM devices. Labels are obtained dynamically from existing GPIO/PWM devices; framebuffer ownership and PWM settings reflect the current state.

### pwmctl

`pwmctl status` shows the PWM settings. `pwmctl freq 2000` sets the common frequency to 2000 Hz; `pwmctl a 50` and `pwmctl b 25` set duty cycles in percent. `pwmctl off` sets both duty cycles to zero. Frequency range: 8–100000 Hz; duty range: 0–100%. Uses existing `/dev/pwm/1/*` devices.
