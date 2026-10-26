# PicoNIX

PicoNIX is a compact UNIX-like educational operating system for the Raspberry Pi Pico (RP2040), developed by TAKAGO_LAB. It is an experiment in operating-system internals rather than a Linux distribution.

Release banner: `PicoNIX-2026-10-26 (TAKAGO_LAB.)`  
Development stage: 30  
License: GPL-3.0-or-later

[日本語](README-JP.md) | [English](README-EN.md)

## Features

- Fixed 64 KiB RAM regions: kernel, Shell, Blink, and a third application slot shared by Omikuji and Tiny Python.
- Preemptive multitasking with SysTick and PendSV; system calls via SVC.
- Process IDs, `ps`, `kill`, and child termination waiting.
- Foreground execution and simple background jobs with `&`.
- USB CDC console (UART0 console can be selected at build time).
- Virtual `/dev` and `/sys/proc` files, plus writable RAM files under `/tmp`.
- Standard input/output redirection using `<`, `>`, and `>>`.
- A deliberately small Python-like interpreter for scripts and REPL use.

## Memory architecture

| RAM address | Purpose |
|---|---|
| `0x20000000` | Kernel, 64 KiB |
| `0x20010000` | Shell, 64 KiB |
| `0x20020000` | Blink, 64 KiB |
| `0x20030000` | Omikuji or Tiny Python, 64 KiB (shared) |

Applications are linked to predetermined addresses and embedded in firmware flash. There is no ELF loader, dynamic linker, relocation, or general process-address isolation. Each application slot reserves its final 4 KiB for a stack.

## Building and starting

Use Pico SDK 2.1.1, an ARM GCC toolchain, CMake, and Python 3 on Linux. The build script defaults to `MinSizeRel` (`-Os`).

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

Copy the generated UF2 file to the Pico in BOOTSEL mode, then connect through USB CDC. See [Setup](SETUP-EN.md) for more details.

## Shell examples

```sh
help
ps
ls /dev
cat /sys/proc/uptime
cat /dev/rand
blink
omikuji 5 &
cd /tmp
echo hello > greeting.txt
cat greeting.txt
echo hello > /dev/uart1
python
```

The `/tmp` filesystem is volatile. The `python` command starts an interactive interpreter; `python /tmp/test.py` executes a RAM-file script. More examples appear in the [Python guide](PYTHON-EN.md).

## Redirection and limitations

Shell redirection supports a single unquoted path for each of standard input and standard output. `<`, `>`, and `>>` are supported; pipes, quoting, and `2>` are not. `spawn()` inherits standard descriptors. `dup2()` copies descriptor state but does **not** implement POSIX shared file offsets. Device output accepts redirection without regular-file creation or truncation flags; device-specific access restrictions still apply.

Background execution is intended for spawned applications, not built-in shell commands. Omikuji and Python cannot execute at the same time because they share slot 3. This operating system does not offer POSIX process protection or a general executable loader.

## Source tree and documentation

`src/kernel.c` provides scheduling, syscalls, device access, and RAM-file storage; `src/context.S` handles context switching; `src/main.c` sets up the system. `ram_apps/` contains independently linked applications and their linker script; `tools/` contains build and validation scripts.

- [Setup instructions](SETUP-EN.md)
- [Internal architecture](INTERNAL-EN.md)
- [Writing applications](PROGRAMMING-EN.md)
- [Tiny Python interpreter](PYTHON-EN.md)

## License

GNU General Public License, version 3 or later. See `LICENSE`.
