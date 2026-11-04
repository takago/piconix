# 目的から探す — PicoNIXドキュメント

「何をしたいか」から関連資料を探せます．各項目には，入門の説明，具体的な操作方法，詳しい仕様などをまとめています．必要なページを選んで開いてください．

**探し方：** 下の分野別一覧から選ぶか，ブラウザのページ内検索（Ctrl+Fなど）で「パイプ」「GPIO」「システムコール」などを探します．

**注意：** Pico Wの通信機能は通常のPicoでは使えません．外部機器を接続する前に，配線と3.3 V系の注意事項を確認してください．

## 目的一覧

### 準備と最初の操作

| やりたいこと | 関連ページ |
|---|---|
| まずPicoNIXを動かしたい | [導入：セットアップ](setup/SETUP.md) · [教科書 A1](learn/A/A1.md) · [仕様：配線](reference/hardware/WIRING.md) |
| ビルドしてUF2を書き込みたい | [導入：セットアップ](setup/SETUP.md) · [教科書 C1](learn/C/C1.md) |
| USB端末に接続したい | [導入：セットアップ](setup/SETUP.md) · [教科書 A1](learn/A/A1.md) · [教科書 B2](learn/B/B2.md) |
| コマンドの使い方や一覧を調べたい | [操作：help](guides/HELP.md) · [教科書 A1](learn/A/A1.md) |
| PicoNIXとLinuxの違いを知りたい | [教科書 A1](learn/A/A1.md) · [仕様：Linuxとの違い](reference/DIFFERENCES.md) · [仕様：制限・仕様](reference/SPECIFICATION.md) |

### シェル・ファイル・パイプ

| やりたいこと | 関連ページ |
|---|---|
| ディレクトリを移動・一覧表示したい | [教科書 A1](learn/A/A1.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| ファイルを作成・コピー・削除したい | [教科書 A2](learn/A/A2.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| ファイルから文字列を探したい（grep） | [教科書 A2](learn/A/A2.md) · [操作：パイプ](guides/PIPE.md) |
| 標準入出力やリダイレクトを使いたい | [教科書 A2](learn/A/A2.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| パイプでコマンドをつなぎたい | [教科書 A2](learn/A/A2.md) · [操作：パイプ](guides/PIPE.md) · [教科書 B1](learn/B/B1.md) |
| edit・viでファイルを編集したい | [操作：edit](guides/EDIT.md) · [操作：vi](guides/VI.md) |
| fdでディレクトリを閲覧したい | [操作：fd](guides/FD.md) · [教科書 A1](learn/A/A1.md) |
| ddでバイナリデータを転送したい | [操作：dd](guides/DD.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| 起動時の/etc/rcや初期ファイルを調べたい | [仕様：初期ファイルとrc](reference/INITIAL-FILES.md) · [教科書 A1](learn/A/A1.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |

### プロセス・端末操作

| やりたいこと | 関連ページ |
|---|---|
| psでプロセスを確認し，killで停止したい | [教科書 A3](learn/A/A3.md) · [教科書 B3](learn/B/B3.md) |
| バックグラウンド実行やCtrl+Cを理解したい | [教科書 A3](learn/A/A3.md) · [実習：TTY設定](guides/TTY.md) |
| 開かれているFDをlsofで確認したい | [教科書 B1](learn/B/B1.md) · [教科書 B4](learn/B/B4.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |
| USBとは別にUART1のシェルを使いたい | [教科書 B2](learn/B/B2.md) · [仕様：UART1](reference/devices/UART.md) |
| sttyやraw/cooked入力を理解したい | [教科書 B2](learn/B/B2.md) · [実習：TTY設定](guides/TTY.md) |
| 画面制御や全画面アプリの動作を知りたい | [操作：端末画面](guides/TERMINAL.md) · [実習：TTY設定](guides/TTY.md) · [教科書 B2](learn/B/B2.md) |

### OS内部を観察する

| やりたいこと | 関連ページ |
|---|---|
| traceでシステムコールを追いたい | [教科書 B4](learn/B/B4.md) · [実習：trace](guides/TRACE.md) · [仕様：システムコール](reference/SYSCALLS.md) |
| memでRAMやタスクのメモリを見たい | [教科書 B3](learn/B/B3.md) · [操作：mem](guides/MEM.md) · [仕様：SRAM配置](reference/MEMORY-LAYOUT.md) |
| FlashとRAMの配置・PICを理解したい | [教科書 B3](learn/B/B3.md) · [仕様：SRAM配置](reference/MEMORY-LAYOUT.md) |
| カーネルのシンボルを調べたい | [仕様：カーネルシンボル](reference/KERNEL-SYMBOLS.md) · [操作：mem](guides/MEM.md) · [索引：ソース関数一覧](../FUNCTIONS.md) |
| VFSとFDの関係を理解したい | [教科書 B1](learn/B/B1.md) · [教科書 C3](learn/C/C3.md) · [仕様：ファイルシステム](reference/FILESYSTEM.md) |

### デバイス・電子工作

| やりたいこと | 関連ページ |
|---|---|
| 使用可能なGPIOピンや配線を調べたい | [仕様：ピン割当](reference/hardware/PINS.md) · [仕様：配線](reference/hardware/WIRING.md) · [教科書 B5](learn/B/B5.md) |
| GPIOの入出力を操作したい | [教科書 B5](learn/B/B5.md) · [仕様：デバイス](reference/devices/DEVICES.md) |
| 8×8 LEDマトリクスを操作したい | [教科書 B5](learn/B/B5.md) · [仕様：デバイス](reference/devices/DEVICES.md) · [仕様：LED配線](reference/hardware/WIRING.md) |
| PWM・ブザー・サーボを試したい | [教科書 B5](learn/B/B5.md) · [仕様：デバイス](reference/devices/DEVICES.md) · [仕様：配線](reference/hardware/WIRING.md) |
| ADCやセンサを読みたい | [教科書 B5](learn/B/B5.md) · [仕様：デバイス](reference/devices/DEVICES.md) · [仕様：配線](reference/hardware/WIRING.md) |
| I²C1センサを接続したい | [操作：I²C](reference/devices/I2C.md) · [仕様：配線](reference/hardware/WIRING.md) · [教科書 B5](learn/B/B5.md) |

### Pico Wのネットワーク（選択）

| やりたいこと | 関連ページ |
|---|---|
| Wi-FiのAP/STA接続を設定したい | [教科書 B6](learn/B/B6.md) · [操作：Pico W接続](reference/network/NETWORK.md) · [仕様：Wi-Fiモード](reference/network/WIFI-MODES.md) |
| IPアドレスや通信状態を確認したい | [教科書 B6](learn/B/B6.md) · [操作：net](reference/network/NET.md) · [仕様：Wi-Fiモード](reference/network/WIFI-MODES.md) |
| Telnetで遠隔シェルを使いたい | [教科書 B6](learn/B/B6.md) · [操作：Pico W接続](reference/network/NETWORK.md) · [操作：net](reference/network/NET.md) |
| ncでTCP/UDP通信を試したい | [教科書 B6](learn/B/B6.md) · [操作：nc](reference/network/NC.md) |
| httpdでWebページを公開したい | [操作：httpd](reference/network/HTTPD.md) · [仕様：初期ファイルとrc](reference/INITIAL-FILES.md) · [教科書 B6](learn/B/B6.md) |
| ping・tcpdumpで通信を観察したい | [教科書 B6](learn/B/B6.md) · [仕様：Pico Wネットワーク](reference/network/NETWORK.md) |
| UDPでデバイスを遠隔制御したい | [仕様：遠隔デバイス制御](reference/network/REMOTE-DEVICE.md) · [実習：遠隔GPIO](reference/network/REMOTE_GPIO.md) · [教科書 B6](learn/B/B6.md) |

### プログラム・カーネルの開発

| やりたいこと | 関連ページ |
|---|---|
| Python風インタプリタを使いたい | [開発：Python](development/apps/python/PYTHON.md) · [教科書 C1](learn/C/C1.md) |
| Thumbアセンブラで試したい | [開発：アセンブラ](development/apps/asm/ASM.md) · [教科書 C1](learn/C/C1.md) |
| 新しいCコマンドを作りたい | [教科書 C1](learn/C/C1.md) · [開発：Cアプリ](development/apps/c/PROGRAMMING.md) · [開発：Cライブラリ](development/apps/c/LIBC.md) |
| Cライブラリの関数を調べたい | [開発：Cライブラリ](development/apps/c/LIBC.md) · [仕様：システムコール](reference/SYSCALLS.md) |
| TCP/UDPソケットをプログラムから使いたい（Pico W） | [開発：ネットワークアプリ](development/apps/c/NETWORK-PROGRAMMING.md) · [教科書 C5](learn/C/C5.md) · [仕様：システムコール](reference/SYSCALLS.md) |
| カーネルを起動順に読みたい | [教科書 C2](learn/C/C2.md) · [読解：カーネル](development/kernel/KERNEL-READING-GUIDE.md) · [仕様：アーキテクチャ](reference/ARCHITECTURE.md) |
| タスク切替とスケジューラを読みたい | [教科書 C2](learn/C/C2.md) · [読解：カーネル](development/kernel/KERNEL-READING-GUIDE.md) · [仕様：内部構造](reference/INTERNAL.md) |
| システムコールを追加したい | [教科書 C3](learn/C/C3.md) · [開発：システムコール追加](development/syscalls/SYSCALL-DEVELOPMENT.md) · [仕様：システムコール](reference/SYSCALLS.md) |
| デバイスドライバを追加したい | [教科書 C4](learn/C/C4.md) · [開発：ドライバ追加](development/drivers/DRIVER-DEVELOPMENT.md) · [仕様：デバイス](reference/devices/DEVICES.md) |
| Wi-Fi・lwIP・ネットワークカーネルを改造したい（Pico W） | [開発：ネットワーク実装](development/network/NETWORK-DEVELOPMENT.md) · [教科書 C5](learn/C/C5.md) · [仕様：ネットワーク](reference/network/README.md) |
| カーネルを小さく改造し検証したい | [教科書 C6](learn/C/C6.md) · [開発：カーネル拡張](development/kernel/KERNEL-DEVELOPMENT.md) · [読解：カーネル](development/kernel/KERNEL-READING-GUIDE.md) |
| 関数名からソースコードを探したい | [索引：ソース関数一覧](../FUNCTIONS.md) · [読解：カーネル](development/kernel/KERNEL-READING-GUIDE.md) |

## ほかの探し方

- **順番に学びたい** → [教科書 A・B・C](learn/README.md)
- **操作例を一覧で探したい** → [補助教材・操作ガイド](guides/README.md)
- **仕様を項目名から探したい** → [技術リファレンス](reference/README.md)
- **ソースの改造手順を探したい** → [開発ガイド](development/README.md)
- **ビルドや書き込みで困っている** → [セットアップ・トラブルシューティング](setup/SETUP.md)

## 通信・カーネル拡張を実際に試したい

| やりたいこと | まず読む | 詳しい実装・参考 |
|---|---|---|
| 自分のIPアドレスを調べる | [`ip`の実習](reference/network/IP.md) | [Wi-Fi接続](reference/network/NETWORK.md) |
| 相手と通信できるか調べる | [`ping`の実習](reference/network/PING.md) | [`net`の状態](reference/network/NET.md) |
| TCP/UDP/ICMPの通信を読み解く | [`tcpdump`の実習](reference/network/TCPDUMP.md) | [教科書B6](learn/B/B6.md) |
| TCP/UDPのCプログラムを自作する | [ソケット実装と2つのCプログラム](development/apps/c/NETWORK-PROGRAMMING.md) | [Cアプリ登録](development/apps/c/PROGRAMMING.md) |
| システムコールを1つ追加する | [getuptime演習](development/syscalls/SYSCALL-DEVELOPMENT.md) | [SVCの仕様](reference/SYSCALLS.md) |
| デバイスノードとドライバを作る | [/dev/uptime演習](development/drivers/DRIVER-DEVELOPMENT.md) | [デバイスの仕様](reference/devices/DEVICES.md) |

---

[English](../en/TASKS.md) ｜ [文書案内](../DOCS.ja.md)
