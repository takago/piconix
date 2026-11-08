# 目的から探す — PicoNIXドキュメント

「何をしたいか」から関連資料を探せます．各項目には，入門の説明，具体的な操作方法，詳しい仕様などをまとめています．必要なページを選んで開いてください．

**探し方：** 下の分野別一覧から選ぶか，ブラウザのページ内検索（Ctrl+Fなど）で「パイプ」「GPIO」「システムコール」などを探します．

**注意：** Pico Wの通信機能は通常のPicoでは使えません．外部機器を接続する前に，配線と3.3 V系の注意事項を確認してください．


## 目的一覧

### 準備と最初の操作

| やりたいこと | 関連ページ |
|---|---|
| まずPicoNIXを動かしたい | [導入：セットアップ](setup/SETUP.md) · [基本操作](reference/commands/HELP.md) · [仕様：配線](reference/hardware/WIRING.md) |
| ビルドしてUF2を書き込みたい | [導入：セットアップ](setup/SETUP.md) · [Cアプリ開発](development/apps/c/PROGRAMMING.md) |
| USB端末に接続したい | [導入：セットアップ](setup/SETUP.md) · [基本操作](reference/commands/HELP.md) · [TTY操作](reference/commands/TTY.md) |
| コマンドの使い方や一覧を調べたい | [操作：help](reference/commands/HELP.md) |
| PicoNIXとLinuxの違いを知りたい | [基本操作](reference/commands/HELP.md) · [仕様：Linuxとの違い](reference/DIFFERENCES.md) · [仕様：制限・仕様](reference/SPECIFICATION.md) |

### シェル・ファイル・パイプ

| やりたいこと | 関連ページ |
|---|---|
| ディレクトリを移動・一覧表示したい | [基本操作](reference/commands/HELP.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| ファイルを作成・コピー・削除したい | [パイプとFD](reference/commands/PIPE.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| ファイルから文字列を探したい（grep） | [パイプとFD](reference/commands/PIPE.md) |
| 標準入出力やリダイレクトを使いたい | [パイプとFD](reference/commands/PIPE.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| パイプでコマンドをつなぎたい | [パイプとFD](reference/commands/PIPE.md) · [ファイルシステム](reference/FILESYSTEM.md) |
| edit・viでファイルを編集したい | [操作：edit](reference/commands/EDIT.md) · [操作：vi](reference/commands/VI.md) |
| fdでディレクトリを閲覧したい | [操作：fd](reference/commands/FD.md) · [基本操作](reference/commands/HELP.md) |
| ddでバイナリデータを転送したい | [操作：dd](reference/commands/DD.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| 起動時の/etc/rcや初期ファイルを調べたい | [仕様：初期ファイルとrc](reference/INITIAL-FILES.md) · [基本操作](reference/commands/HELP.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |

### プロセス・端末操作

| やりたいこと | 関連ページ |
|---|---|
| psでプロセスを確認し，killで停止したい | [システム仕様](reference/SPECIFICATION.md) · [mem操作](reference/commands/MEM.md) |
| バックグラウンド実行やCtrl+Cを理解したい | [システム仕様](reference/SPECIFICATION.md) · [実習：TTY設定](reference/commands/TTY.md) |
| 開かれているFDをlsofで確認したい | [ファイルシステム](reference/FILESYSTEM.md) · [trace操作](reference/commands/TRACE.md) |
| USBとは別にUART1のシェルを使いたい | [TTY操作](reference/commands/TTY.md) · [仕様：UART1](reference/devices/UART.md) |
| sttyやraw/cooked入力を理解したい | [TTY操作](reference/commands/TTY.md) |
| 画面制御や全画面アプリの動作を知りたい | [操作：端末画面](reference/commands/TERMINAL.md) · [実習：TTY設定](reference/commands/TTY.md) |

### OS内部を観察する

| やりたいこと | 関連ページ |
|---|---|
| traceでシステムコールを追いたい | [trace操作](reference/commands/TRACE.md) · [仕様：システムコール](reference/SYSCALLS.md) |
| memでRAMやタスクのメモリを見たい | [mem操作](reference/commands/MEM.md) · [仕様：ROM・Flash・SRAM配置](reference/MEMORY-LAYOUT.md) |
| FlashとRAMの配置・PICを理解したい | [mem操作](reference/commands/MEM.md) · [仕様：ROM・Flash・SRAM配置](reference/MEMORY-LAYOUT.md) |
| カーネルのシンボルを調べたい | [仕様：カーネルシンボル](reference/KERNEL-SYMBOLS.md) · [操作：mem](reference/commands/MEM.md) · [索引：ソース関数一覧](development/FUNCTIONS.md) |
| VFSとFDの関係を理解したい | [ファイルシステム](reference/FILESYSTEM.md) · [システムコール開発](development/syscalls/SYSCALL-DEVELOPMENT.md) |

### デバイス・電子工作

| やりたいこと | 関連ページ |
|---|---|
| 使用可能なGPIOピンや配線を調べたい | [仕様：ピン割当](reference/hardware/PINS.md) · [仕様：配線](reference/hardware/WIRING.md) · [デバイス仕様](reference/devices/DEVICES.md) |
| GPIOの入出力を操作したい | [デバイス仕様](reference/devices/DEVICES.md) |
| `/dev/fbchar`で文字や図柄を表示したい | [デバイス仕様](reference/devices/DEVICES.md) · [ROMマニュアル](reference/commands/HELP.md) |
| `scroll`でLEDへ文字を流したい | [スクロール操作](reference/commands/SCROLL.md) · [デバイス仕様](reference/devices/DEVICES.md) |
| 8×8 LEDマトリクスを操作したい | [デバイス仕様](reference/devices/DEVICES.md) · [仕様：LED配線](reference/hardware/WIRING.md) |
| PWM・ブザー・サーボを試したい | [デバイス仕様](reference/devices/DEVICES.md) · [仕様：配線](reference/hardware/WIRING.md) |
| ADCやセンサを読みたい | [デバイス仕様](reference/devices/DEVICES.md) · [仕様：配線](reference/hardware/WIRING.md) |
| I²C1センサを接続したい | [操作：I²C](reference/devices/I2C.md) · [仕様：配線](reference/hardware/WIRING.md) · [デバイス仕様](reference/devices/DEVICES.md) |

### Pico Wのネットワーク（選択）

| やりたいこと | 関連ページ |
|---|---|
| Wi-FiのAP/STA接続を設定したい | [ネットワーク接続](reference/network/NETWORK.md) · [仕様：Wi-Fiモード](reference/network/WIFI-MODES.md) |
| IPアドレスや通信状態を確認したい | [ネットワーク接続](reference/network/NETWORK.md) · [操作：net](reference/network/NET.md) · [仕様：Wi-Fiモード](reference/network/WIFI-MODES.md) |
| Telnetで遠隔シェルを使いたい | [ネットワーク接続](reference/network/NETWORK.md) · [操作：net](reference/network/NET.md) |
| ncでTCP/UDP通信を試したい | [ネットワーク接続](reference/network/NETWORK.md) · [操作：nc](reference/network/NC.md) |
| httpdでWebページを公開したい | [操作：httpd](reference/network/HTTPD.md) · [仕様：初期ファイルとrc](reference/INITIAL-FILES.md) · [ネットワーク接続](reference/network/NETWORK.md) |
| ローカルLLMからMCPでPicoNIXを操作したい | [MCP統合ガイド](reference/commands/MCP.md) |
| MCPサーバを開発・動作確認したい（Pico W） | [MCP実装・開発資料](development/apps/c/MCP-DEVELOPMENT.md) |
| ping・tcpdumpで通信を観察したい | [`ip`](reference/network/IP.md) · [`ping`](reference/network/PING.md) · [`tcpdump`](reference/network/TCPDUMP.md) |
| UDPでデバイスを遠隔制御したい | [仕様：遠隔デバイス制御](reference/network/REMOTE-DEVICE.md) · [実習：遠隔GPIO](reference/network/REMOTE_GPIO.md) · [ネットワーク接続](reference/network/NETWORK.md) |

### プログラム・カーネルの開発

| やりたいこと | 関連ページ |
|---|---|
| Python風インタプリタを使いたい | [開発：Python](development/apps/python/PYTHON.md) · [Cアプリ開発](development/apps/c/PROGRAMMING.md) |
| Thumbアセンブラで試したい | [開発：アセンブラ](development/apps/asm/ASM.md) · [Cアプリ開発](development/apps/c/PROGRAMMING.md) |
| 新しいCコマンドを作りたい | [Cアプリ開発](development/apps/c/PROGRAMMING.md) · [開発：Cライブラリ](development/apps/c/LIBC.md) |
| Cライブラリの関数を調べたい | [開発：Cライブラリ](development/apps/c/LIBC.md) · [仕様：システムコール](reference/SYSCALLS.md) |
| TCP/UDPソケットをプログラムから使いたい（Pico W） | [開発：ネットワークアプリ](development/apps/c/NETWORK-PROGRAMMING.md) · [ネットワーク開発](development/network/NETWORK-DEVELOPMENT.md) · [仕様：システムコール](reference/SYSCALLS.md) |
| カーネルを起動順に読みたい | [カーネル読解ガイド](development/kernel/KERNEL-READING-GUIDE.md) · [仕様：アーキテクチャ](reference/ARCHITECTURE.md) |
| タスク切替とスケジューラを読みたい | [カーネル読解ガイド](development/kernel/KERNEL-READING-GUIDE.md) · [仕様：内部構造](reference/INTERNAL.md) |
| システムコールを追加したい | [システムコール開発](development/syscalls/SYSCALL-DEVELOPMENT.md) · [仕様：システムコール](reference/SYSCALLS.md) |
| デバイスドライバを追加したい | [ドライバ開発](development/drivers/DRIVER-DEVELOPMENT.md) · [仕様：デバイス](reference/devices/DEVICES.md) |
| Wi-Fi・lwIP・ネットワークカーネルを改造したい（Pico W） | [開発：ネットワーク実装](development/network/NETWORK-DEVELOPMENT.md) · [仕様：ネットワーク](reference/network/README.md) |
| カーネルを小さく改造し検証したい | [カーネル拡張](development/kernel/KERNEL-DEVELOPMENT.md) · [読解：カーネル](development/kernel/KERNEL-READING-GUIDE.md) |
| 関数名からソースコードを探したい | [索引：ソース関数一覧](development/FUNCTIONS.md) · [読解：カーネル](development/kernel/KERNEL-READING-GUIDE.md) |

## ほかの探し方

- **順番に学びたい** → [学習ロードマップ](ROADMAP.md)
- **操作例を一覧で探したい** → [コマンドリファレンス](reference/commands/README.md)
- **仕様を項目名から探したい** → [技術リファレンス](reference/README.md)
- **ソースの改造手順を探したい** → [開発ガイド](development/README.md)
- **ビルドや書き込みで困っている** → [セットアップ・トラブルシューティング](setup/SETUP.md)

## 通信・カーネル拡張を実際に試したい

| やりたいこと | まず読む | 詳しい実装・参考 |
|---|---|---|
| 自分のIPアドレスを調べる | [`ip`の実習](reference/network/IP.md) | [Wi-Fi接続](reference/network/NETWORK.md) |
| 相手と通信できるか調べる | [`ping`の実習](reference/network/PING.md) | [`net`の状態](reference/network/NET.md) |
| TCP/UDP/ICMPの通信を読み解く | [`tcpdump`の実習](reference/network/TCPDUMP.md) | [ネットワーク接続](reference/network/NETWORK.md) |
| TCP/UDPのCプログラムを自作する | [ソケット実装と2つのCプログラム](development/apps/c/NETWORK-PROGRAMMING.md) | [Cアプリ登録](development/apps/c/PROGRAMMING.md) |
| システムコールを1つ追加する | [getuptime演習](development/syscalls/SYSCALL-DEVELOPMENT.md) | [SVCの仕様](reference/SYSCALLS.md) |
| デバイスノードとドライバを作る | [/dev/uptime演習](development/drivers/DRIVER-DEVELOPMENT.md) | [デバイスの仕様](reference/devices/DEVICES.md) |

---

[English](../en/TASKS.md) ｜ [文書案内](README.md)
