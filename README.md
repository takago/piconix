# PicoNIX

**A tiny UNIX-like OS with user-space TCP/IP networking for Raspberry Pi Pico / Pico W**  
**Raspberry Pi Pico／Pico Wで動く，ソケット通信対応の小さなUNIX風OS**

PicoNIXは，RP2040の限られたRAM上で，シェル，マルチタスク，仮想ファイルシステム，デバイスファイル，**ユーザプロセスから利用できるTCP/IPソケット**を実現する実験的なUNIX風OSです．Linuxの移植ではありません．

PicoNIX is an experimental UNIX-like OS for RP2040 microcontrollers. Beyond a shell, multitasking, a virtual file system, and device files, it now offers **TCP/UDP sockets to ordinary user applications**. It is not a Linux port.

## Highlights / 主な特徴

- **User-space TCP/IP networking / ユーザプロセスからのTCP/IP通信** — Pico WでlwIPを利用し，`socket()`，`bind()`，`listen()`，`accept()`，`connect()`，`send()`，`recv()`，`sendto()`，`recvfrom()`を利用できます．TCP/UDPのクライアントとサーバをユーザプログラムとして作成できます． / A compact BSD-style IPv4 socket API lets user programs implement TCP and UDP clients and servers.
- **Networking tools / ネットワークコマンド** — `nc`，`httpd`，`tcp_echod`，`udp_echod`．`httpd`はユーザプロセスで動作し，`/etc/index.html`を配信します． / User-space HTTP and echo servers, plus netcat-like testing.
- **Built-in network services / カーネル標準サービス** — TCP 23のTelnetシェルと，UDP 2000のリモートデバイス制御．サービスは`net_service_telnet.c`と`net_service_remote_devctl.c`に分離しています． / Telnet shell on TCP 23 and remote device control on UDP 2000.
- **UNIX-like environment / UNIX風の環境** — `sh`，`ls`，`cat`，`ps`，`kill`，`find`，`tree`と，`/bin`，`/dev`，`/proc`，`/tmp`，`/etc`． / Shell commands and a small virtual file system.
- **Device files / デバイスファイル** — GPIO，ADC，PWM，UART，LEDマトリクス用フレームバッファをファイル経由で操作．UDP 2000からも対応デバイスを遠隔操作できます． / File-like device access and remote device control.
- **Programming on the Pico / 本体上でプログラミング** — `python`，`as`，`edit`． / Small Python-like interpreter, assembler, and editor.
- **Inspect the OS / OS内部の観察** — `mem`，`trace`，`lsof`（ソケットのポート番号表示に対応）． / Memory viewer, syscall tracing, and open-descriptor inspection.
- **Terminal games / 端末ゲーム** — `mb`（Moon Buggy），`pong`．

## Network demo / ネットワークのデモ

Pico WのWi-FiアクセスポイントにPCを接続します． / Connect your PC to the Pico W access point.

- SSID: `PicoNIX-XXXXXXXX`（デバイスごとに異なります / device-specific suffix）
- Password: `piconix1234`
- Pico W address: `192.168.4.1`

```sh
# Linux PC: PicoNIX shell / PicoNIXのシェルへ接続
telnet 192.168.4.1
```

PicoNIXのシェルで`httpd`を起動すると，PCのブラウザから`http://192.168.4.1/`へアクセスできます．`/etc/index.html`は`edit`で変更できます． / Start `httpd` in the PicoNIX shell, then open `http://192.168.4.1/` in a browser. Edit `/etc/index.html` to change the page.

```sh
# PicoNIX shell
httpd
# Or run a user-space TCP echo server / TCP Echoサーバ
tcp_echod 1234
```

```sh
# Linux PC (while tcp_echod is running)
nc 192.168.4.1 1234
```

UDP echo can be tested with `udp_echod 1234` on PicoNIX and `printf 'hello\n' | nc -u -w 2 192.168.4.1 1234` on Linux.

**Security / セキュリティ：** TelnetとUDP 2000のリモートデバイス制御には暗号化・認証がありません．信頼できる実験用ネットワークだけで利用してください． / Telnet and remote device control are unauthenticated and unencrypted; use only on a trusted lab network.

## Build / ビルド

```sh
# Pico (no Wi-Fi / Wi-Fiなし)
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel

# Pico W (Wi-Fi enabled / Wi-Fiあり)
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

Pico SDKとARMツールチェーンが必要です． / Requires the Pico SDK and an ARM toolchain.

## Architecture / 構成

- `kernel/kernel.c` — タスク，システムコール，VFS / tasks, syscalls, VFS
- `kernel/net.c` — Wi-FiとlwIPの初期化・管理 / Wi-Fi and lwIP integration
- `kernel/net_socket.c` — ユーザプロセス用ソケットバックエンド / user-process socket backend
- `kernel/net_service_telnet.c` — Telnet（TCP 23）
- `kernel/net_service_remote_devctl.c` — リモートデバイス制御（UDP 2000）
- `userland/httpd.c`，`userland/nc.c`，`userland/tcp_echod.c`，`userland/udp_echod.c` — 通常のユーザプログラム / ordinary user applications

ソケットAPIは小規模な実装であり，Linux/POSIXの完全互換ではありません．`send()`と`recv()`のflagsは0のみ対応します． / The socket API is intentionally limited, not fully POSIX-compatible; `send()` and `recv()` support flags=0 only.

## Documentation / ドキュメント

| 日本語 | English |
|---|---|
| [概要・文書一覧](docs/ja/README.md) | [Overview and guides](docs/en/README.md) |
| [セットアップ](docs/ja/SETUP.md) | [Setup](docs/en/SETUP.md) |
| [ネットワーク接続](docs/ja/NETWORK.md) | [Network access](docs/en/NETWORK.md) |
| [ソケットAPI・サーバ](docs/ja/NETWORK-PROGRAMMING.md) | [Sockets and servers](docs/en/NETWORK-PROGRAMMING.md) |
| [Cアプリ開発](docs/ja/PROGRAMMING.md) | [C application development](docs/en/PROGRAMMING.md) |
| [デバイス](docs/ja/DEVICES.md) | [Devices](docs/en/DEVICES.md) |
| [内部構造](docs/ja/INTERNAL.md) | [Internals](docs/en/INTERNAL.md) |

`/tmp`と`/etc`は揮発性RAMファイル領域を共有し，合計最大8ファイル，各1 KiBです． / `/tmp` and `/etc` share up to eight volatile RAM files of 1 KiB each.
