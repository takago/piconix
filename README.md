# PicoNIX

**A tiny UNIX-like operating system for Raspberry Pi Pico / Pico W**  
**Raspberry Pi Pico／Pico Wで動く小さなUNIX風OS**

PicoNIXは，Raspberry Pi Pico（RP2040）およびPico Wで動作する，学習・実験向けの小さなUNIX風オペレーティングシステムです．Linuxの移植版ではなく，限られたメモリでシェル，ファイルシステム，マルチタスク，デバイス操作などを体験できます．

PicoNIX is a small UNIX-like operating system for the Raspberry Pi Pico (RP2040) and Pico W. Built for learning and experimentation rather than as a Linux port, it brings a shell, file system, multitasking, and device access to a resource-constrained microcontroller.

## 主な特徴 / Key features

- **Pico／Pico W対応 / Pico and Pico W support**  
  両方のボードに対応しています．Pico W向けファームウェアが通常のPicoでも動作することを実機で確認しています（Wi-Fi機能は利用不可）．  
  Supports both boards. The Pico W firmware has also been tested on a standard Pico, without Wi-Fi functionality.

- **Wi-FiとTelnet / Wi-Fi and Telnet**  
  Pico W自身がWi-Fiアクセスポイントとなり，外部ルータなしでPCからTelnet接続してシェルを操作できます．生TCP端末にも対応します．  
  Pico W provides its own Wi-Fi access point, allowing a PC to access the shell via Telnet without a router. A raw TCP terminal is also available.

- **UNIX風のコマンドとファイルシステム / UNIX-like commands and file system**  
  `sh`，`ls`，`cat`，`ps`，`kill`，`find`，`tree`などのコマンドと，`/bin`，`/dev`，`/proc`，`/tmp`を備えています．  
  Includes commands such as `sh`, `ls`, `cat`, `ps`, `kill`, `find`, and `tree`, plus `/bin`, `/dev`, `/proc`, and `/tmp`.

- **デバイスファイル / Device files**  
  GPIOやUARTなどのデバイスをファイルとして操作できます．  
  Access devices such as GPIO and UART through file-like interfaces.

- **本体上でプログラミング / On-device programming**  
  `python`，`as`（アセンブラ），`edit`（エディタ）を搭載しています．  
  Includes `python`, `as` (assembler), and `edit` (text editor).

- **OS内部の観察 / Explore OS internals**  
  `mem`，`trace`，`lsof`などでメモリや実行中のシステムを調べられます．  
  Inspect memory and runtime behavior with `mem`, `trace`, and `lsof`.

- **端末ゲーム / Terminal games**  
  `moonbuggy`や`pong`も楽しめます．  
  Includes terminal games such as `moonbuggy` and `pong`.

## Pico WにTelnetで接続 / Connect to Pico W via Telnet

Pico WのアクセスポイントにPCを接続してから，以下を実行します．外部Wi-Fiルータは不要です．  
Connect your PC to the Pico W access point, then run the following. No external Wi-Fi router is required.

- **SSID:** `PicoNIX-XXXXXXXX`（末尾はデバイスごとに異なります / device-specific suffix）
- **Password:** `piconix1234`
- **IP address:** `192.168.4.1`

```sh
telnet 192.168.4.1
```

生TCP接続はポート2323を利用します．USB CDC端末はネットワーク端末と独立しています．  
Raw TCP is also available on port 2323. The USB CDC console remains independent of the network terminal.

**注意 / Security:** Telnetと生TCPは暗号化されません．信頼できる実験環境でのみ使用してください．  
Telnet and raw TCP are unencrypted. Use them only on a trusted experimental network.

詳細 / Details: [日本語：ネットワーク接続](docs/ja/NETWORK.md) | [English: Network access](docs/en/NETWORK.md)

## ビルド / Build

使用するボードに応じて`PICO_BOARD`を指定します．  
Select the target board with `PICO_BOARD`.

```sh
# Raspberry Pi Pico
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel

# Raspberry Pi Pico W
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

上記はPico SDKとツールチェーンのセットアップ後の例です．詳しい手順はセットアップガイドを参照してください．  
These commands assume that the Pico SDK and toolchain are configured. See the setup guides for prerequisites and detailed instructions.

## ドキュメント / Documentation

- [日本語：概要・使い方・文書一覧](docs/ja/README.md)
- [English: Overview, quick start, and documentation](docs/en/README.md)

| 日本語 | English |
|---|---|
| [セットアップ](docs/ja/SETUP.md) | [Setup](docs/en/SETUP.md) |
| [ネットワーク接続](docs/ja/NETWORK.md) | [Network access](docs/en/NETWORK.md) |
| [Cアプリ開発](docs/ja/PROGRAMMING.md) | [C application development](docs/en/PROGRAMMING.md) |
| [デバイス・ピン配置](docs/ja/DEVICES.md) | [Devices and pin assignment](docs/en/DEVICES.md) |
| [内部構造](docs/ja/INTERNAL.md) | [Internals](docs/en/INTERNAL.md) |

`/tmp`のファイル上限は1KiB（最大4ファイル）です．`ls -l`ではファイル種別と，取得可能な場合はサイズを表示します．  
The `/tmp` file limit is 1 KiB per file (up to four files). `ls -l` shows file types and sizes when available.
