# PicoNIX

**A tiny UNIX-like operating system with user-space TCP/IP networking for Raspberry Pi Pico and Pico W.**

PicoNIX is an experimental UNIX-like OS for RP2040 microcontrollers. It combines a shell, multitasking, a virtual file system, device files, and a compact TCP/IP socket interface for ordinary user programs. 

Unlike firmware that exposes only a built-in network service, PicoNIX lets user applications create TCP and UDP clients and servers through familiar socket APIs.

## Highlights

- **XIP kernel** — Kernel instructions execute directly from external Flash through the RP2040 XIP (Execute in Place) mechanism, leaving more SRAM available for runtime data and user processes.
- **Flash-resident PIC applications** — User programs are stored as position-independent code (PIC) images in Flash and copied into fixed RAM slots for execution.
- **Pico W Wi-Fi hardware** — On Pico W, PicoNIX uses the onboard Infineon CYW43439 Wi-Fi chip through the Raspberry Pi Pico SDK, with access-point (AP) and station (STA) modes.
- **Open-source TCP/IP networking** — On Pico W, PicoNIX uses lwIP, a lightweight open-source TCP/IP stack, and provides user applications with a compact BSD-style IPv4 socket API (`socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, and `recvfrom()`).
- **Network applications** — `nc`, `httpd`, `tcp_echod`, and `udp_echod` run as ordinary user programs. The HTTP server serves an editable `/etc/index.html`.
- **Built-in network services** — A Telnet shell on TCP port 23 and remote device control on UDP port 2000 are implemented as separate kernel services.
- **UNIX-like environment** — A shell and familiar commands including `ls`, `cat`, `grep`, `ps`, `kill`, `find`, and `tree`, with `/bin`, `/dev`, `/proc`, `/tmp`, and `/etc`. Binary transfer with `dd` is also supported.
- **Foreground interruption** — Ctrl+C terminates registered foreground tasks, including pipeline stages; `nc -u -N` exits on stdin EOF for one-shot UDP sends.
- **Six application slots** — Six fixed 24 KiB RAM slots (4 KiB stack reserve per slot) on RP2040.
- **Pipes and redirection** — Two- and three-command pipelines using `|`, with `pipe()`, `dup()`, and `dup2()` support.
- **Device files** — Access GPIO, ADC, PWM, I²C1, UART, and an LED-matrix framebuffer through file-like interfaces. Supported devices can also be controlled remotely over UDP.
- **System call interface** — A compact SVC-based ABI exposes process control, file and device I/O, pipes, and TCP/UDP networking. See the [system call reference](docs/en/reference/SYSCALLS.md) ([日本語](docs/ja/reference/SYSCALLS.md)).
- **Programming on the device** — Editors (`edit`, `vi`), an interactive file manager (`fd`), a binary-copy utility (`dd`), an assembler (`as`), and a small Python-like interpreter (`python`).
- **Inspect the OS** — `mem`, `trace`, and `lsof` provide memory inspection, system-call and context-switch tracing, and open-descriptor reporting, including socket ports.
- **Terminal games** — Moon Buggy (`mb`) and `pong`.

## Commands

PicoNIX includes familiar UNIX-style utilities as well as tools for inspecting the OS, programming the RP2040, and experimenting with networking. Run `help` in the PicoNIX shell for command descriptions.

| Category | Commands |
|---|---|
| Shell & files | `sh`, `ls`, `cat`, `grep`, `pwd`, `echo`, `rm`, `find`, `tree`, `fd`, `dd` |
| Processes & diagnostics | `ps`, `kill`, `lsof`, `trace`, `mem`, `hexdump`, `watch` |
| Terminals | `getty`, `tty`, `stty`, `reset` |
| Programming | `edit`, `vi`, `as`, `python` |
| Hardware | `gpio`, `pins`, `fbctl`, `fbed`, `pwmctl`, `i2cctl`, `bh1750`, `blink` |
| Networking | `net`, `nc`, `httpd`, `tcp_echod`, `udp_echod` |
| Games & demos | `mb`, `pong`, `omikuji`, `hello`, `kitesys` |
| Help | `help` |

Shell features include redirection (`<`, `>`) and pipelines (`|`); these are shell operators rather than separate commands. On Pico W, network applications use the user-space socket API. See the [programming guide](docs/en/development/apps/c/PROGRAMMING.md) and [日本語の開発ガイド](docs/ja/development/apps/c/PROGRAMMING.md) for more details.

## Network demo

Connect a Linux PC to the Pico W access point:

- SSID: `PicoNIX-XXXXXXXX` (device-specific suffix)
- Password: `piconix1234`
- Pico W address: `192.168.4.1`
- Linux PC address: `192.168.4.2` (single-client DHCP configuration; verify the assigned address)

These addresses apply to AP mode. In STA mode, use the addresses assigned by the existing network.

Open the PicoNIX shell from the PC:

```sh
telnet 192.168.4.1
```

The Pico W `/etc/rc` starts `httpd` automatically. Visit `http://192.168.4.1/` in a browser; `/` is the introduction page, `/fbcheck.html` controls the LED matrix, and `/pwm.html` controls PWM. To change the introduction page, copy it to `/tmp` with `dd if=/etc/index.html of=/tmp/index.html`, edit `/tmp/index.html`, and copy it back with `dd if=/tmp/index.html of=/etc/index.html`. The editors only open `/tmp` files, and RAM changes are lost on reboot. If `httpd` has been stopped, restart it from the shell:

```sh
# PicoNIX shell
httpd
```

Alternatively, start a TCP echo server and connect from Linux:

```sh
# PicoNIX shell
tcp_echod 1234
```

```sh
# Linux PC
nc 192.168.4.1 1234
```

For UDP, start `udp_echod 1234` on PicoNIX and run:

```sh
printf 'hello\n' | nc -u -w 2 192.168.4.1 1234
```

To send a one-shot UDP message in the other direction, run `nc -u -l 12345` on Linux and `echo hello | nc -u -N 192.168.4.2 12345` on PicoNIX.

**Security:** The built-in Telnet and remote device-control services do not provide authentication or encryption. Use them only on a trusted experimental network.

## Build

```sh
# Raspberry Pi Pico (without Wi-Fi)
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel

# Raspberry Pi Pico W (with Wi-Fi)
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

The Pico SDK and an ARM toolchain are required. See the setup guide linked below for installation details. The firmware has not been verified on hardware by this documentation review.

## Architecture

### Execution and memory model

- **Kernel code:** Executes directly from external Flash using XIP.
- **User programs:** Stored in Flash as PIC images; the selected image is copied into a RAM application slot when launched and executes from SRAM.
- **SRAM:** Holds kernel data and stacks, user-process code/data/stacks, and other runtime state. PicoNIX uses six fixed 24 KiB application slots on RP2040.

This approach saves scarce SRAM by keeping kernel instructions in Flash while allowing user applications to run from separate RAM slots. See [Architecture](docs/en/reference/ARCHITECTURE.md) and [全体構造](docs/ja/reference/ARCHITECTURE.md) for details.

| Component | Responsibility |
|---|---|
| `kernel/kernel.c` | Tasks, system calls, and VFS |
| `kernel/net.c` | Wi-Fi and lwIP initialization and management |
| `kernel/net_socket.c` | Socket backend for user processes |
| `kernel/net_service_telnet.c` | Built-in Telnet service (TCP 23) |
| `kernel/net_service_remote_devctl.c` | Remote device-control service (UDP 2000) |
| `userland/apps/nc.c`, `userland/apps/httpd.c` | User-space networking applications |
| `userland/apps/tcp_echod.c`, `userland/apps/udp_echod.c` | User-space echo servers |

The socket API is intentionally small rather than fully POSIX-compatible. `send()` and `recv()` currently support `flags = 0` only.

The volatile `/tmp` and `/etc` directories share a pool of up to eleven RAM files, each limited to 2 KiB. A separate 2 KiB region is exposed as `/dev/shm`; on Pico W, lwIP has a 10 KiB heap inside the 16 KiB network reservation and supports loopback to `127.0.0.1`. Use `cat /proc/net/memory` to inspect network allocation statistics. PicoNIX does not provide `ping` or `curl` commands (the examples in the HTTP guide run `curl` on a host PC). Initial files come from `userland/fs/` at build time; the PID 1 shell runs `/etc/rc` once on boot.

## Documentation / ドキュメント

**Start here ：** [Documentation Guide](docs/DOCS.md) / [文書案内](docs/DOCS.ja.md) — recommended reading order and links to detailed references.

### Setup

| English | 日本語 |
|---|---|
| [Setup](docs/en/setup/SETUP.md) | [セットアップ](docs/ja/setup/SETUP.md) |
| [Network access](docs/en/reference/devices/NETWORK.md) | [ネットワーク接続](docs/ja/reference/devices/NETWORK.md) |
| [Wi-Fi modes](docs/en/reference/devices/WIFI-MODES.md) | [Wi-Fiモード](docs/ja/reference/devices/WIFI-MODES.md) |
| [Initial RAM files and /etc/rc](docs/en/reference/INITIAL-FILES.md) | [初期RAMファイルと /etc/rc](docs/ja/reference/INITIAL-FILES.md) |

### Understanding PicoNIX

| English | 日本語 |
|---|---|
| [Overview and guides](docs/en/literacy/README.md) | [概要・文書一覧](docs/ja/literacy/README.md) |
| [Development diary](docs/DEVELOPMENT_DIARY.md) | [開発日誌](docs/DEVELOPMENT_DIARY.ja.md) |
| [Architecture](docs/en/reference/ARCHITECTURE.md) | [全体構造](docs/ja/reference/ARCHITECTURE.md) |
| [Internals](docs/en/reference/INTERNAL.md) | [内部構造](docs/ja/reference/INTERNAL.md) |
| [Pipes and standard I/O](docs/en/literacy/PIPE.md) | [パイプと標準入出力](docs/ja/literacy/PIPE.md) |
| [Terminal and screen](docs/en/literacy/TERMINAL.md) | [端末と画面制御](docs/ja/literacy/TERMINAL.md) |
| [TTY settings](docs/en/literacy/TTY.md) | [TTY設定](docs/ja/literacy/TTY.md) |
| [Devices and pin assignments](docs/en/reference/devices/DEVICES.md) | [デバイスとピン割り当て](docs/ja/reference/devices/DEVICES.md) |
| [I²C1 and BH1750](docs/en/reference/devices/I2C.md) | [I²C1とBH1750](docs/ja/reference/devices/I2C.md) |
| [System-call and context-switch tracing](docs/en/literacy/TRACE.md) | [システムコール・コンテキストスイッチのトレース](docs/ja/literacy/TRACE.md) |
| [Memory inspection](docs/en/literacy/MEM.md) | [メモリ参照](docs/ja/literacy/MEM.md) |
| [SRAM layout and shared memory](docs/en/reference/MEMORY-LAYOUT.md) | [SRAMレイアウトと共有メモリ](docs/ja/reference/MEMORY-LAYOUT.md) |

### Development

| English | 日本語 |
|---|---|
| [C application development](docs/en/development/apps/c/PROGRAMMING.md) | [Cアプリ開発](docs/ja/development/apps/c/PROGRAMMING.md) |
| [Kernel development](docs/en/development/kernel/KERNEL-DEVELOPMENT.md) | [カーネル拡張ガイド](docs/ja/development/kernel/KERNEL-DEVELOPMENT.md) |
| [Assembly programming](docs/en/development/apps/asm/ASM.md) | [アセンブリプログラミング](docs/ja/development/apps/asm/ASM.md) |
| [C library and APIs](docs/en/development/apps/c/LIBC.md) | [CライブラリとAPI](docs/ja/development/apps/c/LIBC.md) |
| [System call reference](docs/en/reference/SYSCALLS.md) | [システムコール一覧](docs/ja/reference/SYSCALLS.md) |
| [Sockets and servers](docs/en/development/apps/c/NETWORK-PROGRAMMING.md) | [ソケットAPI・サーバ](docs/ja/development/apps/c/NETWORK-PROGRAMMING.md) |
| [Remote device control (UDP 2000)](docs/en/reference/devices/REMOTE-DEVICE.md) | [リモートデバイス制御（UDP 2000番）](docs/ja/reference/devices/REMOTE-DEVICE.md) |
| [nc and termination](docs/en/reference/devices/NC.md) | [ncと通信の終了](docs/ja/reference/devices/NC.md) |
| [Text editing](docs/en/literacy/EDIT.md) | [テキスト編集](docs/ja/literacy/EDIT.md) |
| [vi editor](docs/en/literacy/VI.md) | [viエディタ](docs/ja/literacy/VI.md) |
| [fd file manager](docs/en/literacy/FD.md) | [fdファイルマネージャ](docs/ja/literacy/FD.md) |
| [dd and binary transfer](docs/en/literacy/DD.md) | [ddとバイナリ転送](docs/ja/literacy/DD.md) |
| [Tiny Python](docs/en/development/apps/python/PYTHON.md) | [簡易Python](docs/ja/development/apps/python/PYTHON.md) |

Boot RAM files and startup script: [Japanese](docs/ja/reference/INITIAL-FILES.md) / [English](docs/en/reference/INITIAL-FILES.md).
