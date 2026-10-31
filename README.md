# PicoNIX

**A tiny UNIX-like operating system with user-space TCP/IP networking for Raspberry Pi Pico and Pico W.**

PicoNIX is an experimental UNIX-like OS for RP2040 microcontrollers. It combines a shell, multitasking, a virtual file system, device files, and a compact TCP/IP socket interface for ordinary user programs. It is **not a Linux port**.

Unlike firmware that exposes only a built-in network service, PicoNIX lets user applications create TCP and UDP clients and servers through familiar socket APIs.

## Highlights

- **User-space TCP/IP networking** — A compact BSD-style IPv4 socket interface provides `socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, and `recvfrom()` on Pico W.
- **Network applications** — `nc`, `httpd`, `tcp_echod`, and `udp_echod` run as ordinary user programs. The HTTP server serves an editable `/etc/index.html`.
- **Built-in network services** — A Telnet shell on TCP port 23 and remote device control on UDP port 2000 are implemented as separate kernel services.
- **UNIX-like environment** — A shell and familiar commands including `ls`, `cat`, `ps`, `kill`, `find`, and `tree`, with `/bin`, `/dev`, `/proc`, `/tmp`, and `/etc`.
- **Device files** — Access GPIO, ADC, PWM, UART, and an LED-matrix framebuffer through file-like interfaces. Supported devices can also be controlled remotely over UDP.
- **Programming on the device** — An editor (`edit`), assembler (`as`), and small Python-like interpreter (`python`).
- **Inspect the OS** — `mem`, `trace`, and `lsof` provide memory inspection, system-call tracing, and open-descriptor reporting, including socket ports.
- **Terminal games** — Moon Buggy (`mb`) and `pong`.

## Network demo

Connect a Linux PC to the Pico W access point:

- SSID: `PicoNIX-XXXXXXXX` (device-specific suffix)
- Password: `piconix1234`
- Pico W address: `192.168.4.1`

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

**Security:** The built-in Telnet and remote device-control services do not provide authentication or encryption. Use them only on a trusted experimental network.

## Build

```sh
# Raspberry Pi Pico (without Wi-Fi)
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel

# Raspberry Pi Pico W (with Wi-Fi)
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

The Pico SDK and an ARM toolchain are required. See the setup guide below for installation details.

## Architecture

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

Detailed guides are available in English and Japanese．詳細な説明は英語版と日本語版のドキュメントを参照してください．

| 日本語 | English |
|---|---|
| [概要・文書一覧](docs/ja/README.md) | [Overview and guides](docs/en/README.md) |
| [セットアップ](docs/ja/SETUP.md) | [Setup](docs/en/SETUP.md) |
| [ネットワーク接続](docs/ja/NETWORK.md) | [Network access](docs/en/NETWORK.md) |
| [ソケットAPI・サーバ](docs/ja/NETWORK-PROGRAMMING.md) | [Sockets and servers](docs/en/NETWORK-PROGRAMMING.md) |
| [Cアプリ開発](docs/ja/PROGRAMMING.md) | [C application development](docs/en/PROGRAMMING.md) |
| [デバイス](docs/ja/DEVICES.md) | [Devices](docs/en/DEVICES.md) |
| [内部構造](docs/ja/INTERNAL.md) | [Internals](docs/en/INTERNAL.md) |
