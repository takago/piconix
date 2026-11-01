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
- **UNIX-like environment** — A shell and familiar commands including `ls`, `cat`, `grep`, `ps`, `kill`, `find`, and `tree`, with `/bin`, `/dev`, `/proc`, `/tmp`, and `/etc`.
- **Foreground interruption** — Ctrl+C terminates registered foreground tasks, including pipeline stages; `nc -u -N` exits on stdin EOF for one-shot UDP sends.
- **Six application slots** — Six fixed 24 KiB RAM slots (4 KiB stack reserve per slot) on RP2040.
- **Pipes and redirection** — Two- and three-command pipelines using `|`, with `pipe()`, `dup()`, and `dup2()` support.
- **Device files** — Access GPIO, ADC, PWM, UART, and an LED-matrix framebuffer through file-like interfaces. Supported devices can also be controlled remotely over UDP.
- **System call interface** — A compact SVC-based ABI exposes process control, file and device I/O, pipes, and TCP/UDP networking. See the [system call reference](docs/en/SYSCALLS.md) ([日本語](docs/ja/SYSCALLS.md)).
- **Programming on the device** — An editor (`edit`), assembler (`as`), and small Python-like interpreter (`python`).
- **Inspect the OS** — `mem`, `trace`, and `lsof` provide memory inspection, system-call and context-switch tracing, and open-descriptor reporting, including socket ports.
- **Terminal games** — Moon Buggy (`mb`) and `pong`.

## Commands

PicoNIX includes familiar UNIX-style utilities as well as tools for inspecting the OS, programming the RP2040, and experimenting with networking. Run `help` in the PicoNIX shell for command descriptions.

| Category | Commands |
|---|---|
| Shell & files | `sh`, `ls`, `cat`, `grep`, `pwd`, `echo`, `rm`, `find`, `tree` |
| Processes & diagnostics | `ps`, `kill`, `lsof`, `trace`, `mem`, `hexdump`, `watch` |
| Terminals | `tty`, `stty`, `reset` |
| Programming | `edit`, `as`, `python` |
| Hardware | `gpio`, `pins`, `fbctl`, `fbed`, `pwmctl`, `blink` |
| Networking | `nc`, `httpd`, `tcp_echod`, `udp_echod` |
| Games & demos | `mb`, `pong`, `omikuji`, `hello`, `kitesys` |
| Help | `help` |

Shell features include redirection (`<`, `>`) and pipelines (`|`); these are shell operators rather than separate commands. On Pico W, network applications use the user-space socket API. See the [programming guide](docs/en/PROGRAMMING.md) and [日本語の開発ガイド](docs/ja/PROGRAMMING.md) for more details.

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

Start the HTTP server in the PicoNIX shell, then visit `http://192.168.4.1/` in a browser. Use `edit /etc/index.html` to change the page.

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

This approach saves scarce SRAM by keeping kernel instructions in Flash while allowing user applications to run from separate RAM slots. See [Architecture](docs/en/ARCHITECTURE.md) and [全体構造](docs/ja/ARCHITECTURE.md) for details.

| Component | Responsibility |
|---|---|
| `kernel/kernel.c` | Tasks, system calls, and VFS |
| `kernel/net.c` | Wi-Fi and lwIP initialization and management |
| `kernel/net_socket.c` | Socket backend for user processes |
| `kernel/net_service_telnet.c` | Built-in Telnet service (TCP 23) |
| `kernel/net_service_remote_devctl.c` | Remote device-control service (UDP 2000) |
| `userland/nc.c`, `userland/httpd.c` | User-space networking applications |
| `userland/tcp_echod.c`, `userland/udp_echod.c` | User-space echo servers |

The socket API is intentionally small rather than fully POSIX-compatible. `send()` and `recv()` currently support `flags = 0` only.

The volatile `/tmp` and `/etc` directories share a pool of up to eight RAM files, each limited to 1 KiB.

## Documentation / ドキュメント

**Start here / まずはこちら：** [Documentation Guide](docs/en/DOCS.md) / [文書案内](docs/ja/DOCS.md) — recommended reading order and links to detailed references.

| English | 日本語 |
|---|---|
| [Overview and guides](docs/en/README.md) | [概要・文書一覧](docs/ja/README.md) |
| [Setup](docs/en/SETUP.md) | [セットアップ](docs/ja/SETUP.md) |
| [Network access](docs/en/NETWORK.md) | [ネットワーク接続](docs/ja/NETWORK.md) |
| [Wi-Fi modes](docs/en/WIFI-MODES.md) | [Wi-Fiモード](docs/ja/WIFI-MODES.md) |
| [Sockets and servers](docs/en/NETWORK-PROGRAMMING.md) | [ソケットAPI・サーバ](docs/ja/NETWORK-PROGRAMMING.md) |
| [Pipes and standard I/O](docs/en/PIPE.md) | [パイプと標準入出力](docs/ja/PIPE.md) |
| [nc and termination](docs/en/NC.md) | [ncと通信の終了](docs/ja/NC.md) |
| [C application development](docs/en/PROGRAMMING.md) | [Cアプリ開発](docs/ja/PROGRAMMING.md) |
| [Assembly programming](docs/en/ASM.md) | [アセンブリプログラミング](docs/ja/ASM.md) |
| [C library and APIs](docs/en/LIBC.md) | [CライブラリとAPI](docs/ja/LIBC.md) |
| [System call reference](docs/en/SYSCALLS.md) | [システムコール一覧](docs/ja/SYSCALLS.md) |
| [System-call and context-switch tracing](docs/en/TRACE.md) | [システムコール・コンテキストスイッチのトレース](docs/ja/TRACE.md) |
| [Text editing](docs/en/EDIT.md) | [テキスト編集](docs/ja/EDIT.md) |
| [Memory inspection](docs/en/MEM.md) | [メモリ参照](docs/ja/MEM.md) |
| [Devices](docs/en/DEVICES.md) | [デバイス](docs/ja/DEVICES.md) |
| [Architecture](docs/en/ARCHITECTURE.md) | [全体構造](docs/ja/ARCHITECTURE.md) |
| [Terminal and screen](docs/en/TERMINAL.md) | [端末と画面制御](docs/ja/TERMINAL.md) |
| [TTY settings](docs/en/TTY.md) | [TTY設定](docs/ja/TTY.md) |
| [Internals](docs/en/INTERNAL.md) | [内部構造](docs/ja/INTERNAL.md) |
