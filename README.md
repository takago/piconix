# PicoNIX

**A tiny UNIX-like operating system with user-space TCP/IP networking for Raspberry Pi Pico and Pico W.**

PicoNIX is an experimental UNIX-like OS for RP2040 microcontrollers. It combines a shell, multitasking, a virtual file system, device files, and a compact TCP/IP socket interface for ordinary user programs. 

Unlike firmware that exposes only a built-in network service, PicoNIX lets user applications create TCP and UDP clients and servers through familiar socket APIs.

**Development and authorship.** The design principles and specifications of PicoNIX are determined by its creator based on their knowledge and experience. The source code and documentation are generated and refined using ChatGPT in accordance with these decisions. The creator reviews the results and carries out build and hardware verification as development proceeds. See the [development diary](docs/DEVELOPMENT_DIARY.md) ([日本語](docs/DEVELOPMENT_DIARY.ja.md)) for the development process.

**開発体制について．** PicoNIXの設計方針と仕様は，作成者の知識と経験に基づいて決定しています．ソースコードとドキュメントは，これらの方針に従ってChatGPTを用いて生成・改良したものです．開発の経緯は[開発日誌](docs/DEVELOPMENT_DIARY.ja.md)をご覧ください．

## Highlights

- **XIP kernel** — Kernel instructions execute directly from external Flash through the RP2040 XIP (Execute in Place) mechanism, leaving more SRAM available for runtime data and user processes.
- **Flash-resident PIC applications** — User programs are stored as position-independent code (PIC) images in Flash and copied into fixed RAM slots for execution.
- **Pico W Wi-Fi hardware** — On Pico W, PicoNIX uses the onboard Infineon CYW43439 Wi-Fi chip through the Raspberry Pi Pico SDK, with access-point (AP) and station (STA) modes.
- **Open-source TCP/IP networking** — On Pico W, PicoNIX uses lwIP, a lightweight open-source TCP/IP stack, and provides user applications with a compact BSD-style IPv4 socket API (`socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, and `recvfrom()`).
- **Network applications** — `nc`, `httpd`, `tcp_echod`, and `udp_echod` run as ordinary user programs. The HTTP server serves an editable `/etc/index.html`.
- **Built-in network services** — A Telnet shell on TCP port 23 and remote device control on UDP port 2000 are implemented as separate kernel services.
- **UNIX-like environment** — A shell and familiar commands including `ls`, `cat`, `cp`, `grep`, `ps`, `kill`, `find`, and `tree`, with `/bin`, `/dev`, `/proc`, `/tmp`, and `/etc`. Binary transfer with `dd` is also supported.
- **Foreground interruption** — Ctrl+C terminates registered foreground tasks, including pipeline stages; `nc -u -N` exits on stdin EOF for one-shot UDP sends.
- **Six application slots** — Six fixed 24 KiB RAM slots (4 KiB stack reserve per slot) on RP2040.
- **Pipes and redirection** — Two- and three-command pipelines using `|`, with `pipe()`, `dup()`, and `dup2()` support.
- **Device files** — Access GPIO, ADC, PWM, I²C1, UART, and an LED-matrix framebuffer through file-like interfaces. Supported devices can also be controlled remotely over UDP.
- **System call interface** — A compact SVC-based ABI exposes process control, file and device I/O, pipes, and TCP/UDP networking. See the [system call reference](docs/en/reference/SYSCALLS.md) ([日本語](docs/ja/reference/SYSCALLS.md)).
- **Programming on the device** — Editors (`edit`, `vi`), an interactive file manager (`fd`), a binary-copy utility (`dd`), an assembler (`as`), and a small Python-like interpreter (`python`).
- **Inspect the OS** — `mem`, `trace`, and `lsof` provide memory inspection, system-call and context-switch tracing, and open-descriptor reporting, including socket ports.
- **Terminal games** — Moon Buggy (`mb`) and `pong`.

## Commands

PicoNIX includes familiar UNIX-style utilities as well as tools for inspecting the OS, programming the RP2040, and experimenting with networking. Run `help` for the command list or `help NAME` for usage. For example, `cat -n /tmp/test.py` displays line numbers.

| Category | Commands |
|---|---|
| Shell & files | `sh`, `ls`, `cat`, `cp`, `grep`, `pwd`, `echo`, `rm`, `find`, `tree`, `fd`, `dd` |
| Processes & diagnostics | `ps`, `kill`, `lsof`, `trace`, `mem`, `hexdump`, `watch` |
| Terminals | `getty`, `tty`, `stty`, `reset` |
| Programming | `edit`, `vi`, `as`, `python` |
| Hardware | `gpio`, `pins`, `fbctl`, `fbed`, `pwmctl`, `i2cctl`, `bh1750`, `blink` |
| Networking | `ip`, `net`, `nc`, `ping`, `tcpdump`, `httpd`, `tcp_echod`, `udp_echod` |
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

The Pico W `/etc/rc` starts `httpd` automatically. Visit `http://192.168.4.1/` in a browser; `/` is the introduction page, `/fbcheck.html` controls the LED matrix, and `/pwm.html` controls PWM. To change the introduction page, run `edit /etc/index.html` (or `vi /etc/index.html`) and save. The editors open RAM files directly under `/tmp` and `/etc`; changes are lost on reboot. If `httpd` has been stopped, restart it from the shell:

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

The Pico SDK and an ARM toolchain are required. See the setup guide linked below for installation details.

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
| `kernel/net_capture.c` | IPv4 packet summaries and ICMP Echo for Pico W |
| `kernel/net_service_remote_devctl.c` | Remote device-control service (UDP 2000) |
| `userland/apps/nc.c`, `userland/apps/httpd.c` | User-space networking applications |
| `userland/apps/tcp_echod.c`, `userland/apps/udp_echod.c` | User-space echo servers |

The socket API is intentionally small rather than fully POSIX-compatible. `send()` and `recv()` currently support `flags = 0` only.

The volatile `/tmp` and `/etc` directories share a pool of up to eleven RAM files, each limited to 2 KiB. A separate 2 KiB region is exposed as `/dev/shm`; on Pico W, lwIP has a 10 KiB heap inside the 16 KiB network reservation and supports loopback to `127.0.0.1`. Use `cat /proc/net/memory` to inspect network allocation statistics. PicoNIX provides `ping` and `tcpdump` on Pico W, but does not provide a `curl` command (the examples in the HTTP guide run `curl` on a host PC). Initial files come from `userland/fs/` at build time; the PID 1 shell runs `/etc/rc` once on boot.

## Documentation

Choose the documentation that matches your purpose. Start with **Setup** if you are new to PicoNIX. The technical references below link directly to the most frequently consulted specifications.

| Category | English | 日本語 |
|---|---|---|
| **Setup** — Install the development tools | [Setup guide](docs/en/setup/SETUP.md) | [セットアップ手順](docs/ja/setup/SETUP.md) |
| **Learning roadmap** — Suggested order for learning UNIX | [Learning roadmap](docs/en/ROADMAP.md) | [学習ロードマップ](docs/ja/ROADMAP.md) |
| **Task index** — Find instructions by goal | [Task index](docs/en/TASKS.md) | [目的別索引](docs/ja/TASKS.md) |
| **Usage guides** — Commands and practical examples | [Usage guides](docs/en/guides/README.md) | [操作ガイド](docs/ja/guides/README.md) |
| **System specifications** — Features and limitations | [Specifications](docs/en/reference/SPECIFICATION.md) | [システム仕様](docs/ja/reference/SPECIFICATION.md) |
| **Architecture** — Kernel and process design | [Architecture](docs/en/reference/ARCHITECTURE.md) | [システム構成](docs/ja/reference/ARCHITECTURE.md) |
| **System calls** — ABI and call definitions | [System calls](docs/en/reference/SYSCALLS.md) | [システムコール一覧](docs/ja/reference/SYSCALLS.md) |
| **C library** — User-space C interfaces | [C library](docs/en/development/apps/c/LIBC.md) | [Cライブラリ](docs/ja/development/apps/c/LIBC.md) |
| **Memory layout** — Flash and RAM allocation | [Memory layout](docs/en/reference/MEMORY-LAYOUT.md) | [メモリ配置](docs/ja/reference/MEMORY-LAYOUT.md) |
| **Hardware** — Boards, pins and wiring | [Hardware](docs/en/reference/hardware/README.md) | [ハードウェア](docs/ja/reference/hardware/README.md) |
| **Devices** — Device files and interfaces | [Devices](docs/en/reference/devices/README.md) | [デバイス仕様](docs/ja/reference/devices/README.md) |
| **Networking** — Wi-Fi, TCP/IP and services | [Networking](docs/en/reference/network/README.md) | [ネットワーク仕様](docs/ja/reference/network/README.md) |
| **Development guides** — Applications and kernel extensions | [Development guides](docs/en/development/README.md) | [開発ガイド](docs/ja/development/README.md) |
| **Function index** — Browse source functions (Japanese only) | [Function index (Japanese)](docs/FUNCTIONS.md) | [関数索引](docs/FUNCTIONS.md) |
| **Development diary** — Project development history | [Development diary](docs/DEVELOPMENT_DIARY.md) | [開発日誌](docs/DEVELOPMENT_DIARY.ja.md) |

The learning roadmap gives the recommended sequence; usage guides and development guides provide the hands-on instructions. The reference pages describe the interfaces and constraints.
