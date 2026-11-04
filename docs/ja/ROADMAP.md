# UNIX学習ロードマップ

PicoNIXを使って，UNIXの**操作 → 仕組みの観察 → ソースコードの読解 → 改造**へ進むための道案内です．このページは学習ロードマップの本文ではなく，詳しい説明と実習を収録した既存のガイドへ順番に案内します．

はじめて利用する場合は[セットアップ手順](setup/SETUP.md)で開発環境を整えてください．各項目は，上から順に進めることを推奨します．操作の詳細やトラブル対応はリンク先を参照してください．Pico Wが必要な項目は「選択」と明記しています．

## 1．まず使ってみる

| 順序 | 学ぶこと・試すこと | 読む資料 |
|---|---|---|
| 1 | PicoNIXの導入とUSB端末接続 | [セットアップ](setup/SETUP.md)・[helpと基本操作](guides/HELP.md) |
| 2 | ファイルやディレクトリを調べる | [ファイルシステム](reference/FILESYSTEM.md)・[ファイルの編集](guides/EDIT.md) |
| 3 | コマンドの出力をつなぐ | [パイプとFD](guides/PIPE.md) |
| 4 | プロセスと端末を扱う | [TTY・stty](guides/TTY.md)・[端末画面](guides/TERMINAL.md) |

**確認してみること：** `help`でコマンドを調べ，`ls /bin`で実行できるプログラムを探し，`echo hello | cat`を実行してください．入出力がどのようにつながるか説明してみましょう．

## 2．OSの内部を観察する

| 順序 | 学ぶこと・試すこと | 読む資料 |
|---|---|---|
| 5 | ファイルとFD，VFSの関係 | [ファイルシステム](reference/FILESYSTEM.md)・[パイプ](guides/PIPE.md) |
| 6 | メモリとユーザプロセスの配置 | [mem操作ガイド](guides/MEM.md)・[SRAM配置](reference/MEMORY-LAYOUT.md) |
| 7 | システムコールを追跡する | [trace操作ガイド](guides/TRACE.md)・[システムコール一覧](reference/SYSCALLS.md) |
| 8 | GPIOなどのデバイス操作 | [デバイス仕様](reference/devices/DEVICES.md)・[ピン割当](reference/hardware/PINS.md)・[配線](reference/hardware/WIRING.md) |
| 9（選択） | Pico Wの通信を観察する | [ネットワーク接続](reference/network/NETWORK.md)・[ip](reference/network/IP.md)・[ping](reference/network/PING.md)・[tcpdump](reference/network/TCPDUMP.md) |

**確認してみること：** `trace run hello`でシステムコールを観察し，[traceの説明](guides/TRACE.md)に従って`ENTER`と`EXIT`の意味を確認してください．`mem`は[専用ガイド](guides/MEM.md)を読んでから操作してください．

## 3．プログラムとカーネルを改造する

| 順序 | 学ぶこと・試すこと | 読む資料 |
|---|---|---|
| 10 | Cのユーザプログラムを書く | [Cアプリ開発](development/apps/c/PROGRAMMING.md)・[ライブラリAPI](development/apps/c/LIBC.md) |
| 11 | カーネルの起動とタスク管理を追う | [カーネル読解ガイド](development/kernel/KERNEL-READING-GUIDE.md)・[内部構造](reference/INTERNAL.md) |
| 12 | システムコールを追加する | [システムコール開発](development/syscalls/SYSCALL-DEVELOPMENT.md) |
| 13 | デバイスノードとドライバを追加する | [ドライバ開発](development/drivers/DRIVER-DEVELOPMENT.md) |
| 14（選択） | TCP/UDPアプリとネットワーク実装を調べる | [ソケットプログラミング](development/apps/c/NETWORK-PROGRAMMING.md)・[ネットワーク開発](development/network/NETWORK-DEVELOPMENT.md) |
| 15 | カーネル変更を検証する | [カーネル拡張ガイド](development/kernel/KERNEL-DEVELOPMENT.md) |

**確認してみること：** ガイドにある小さなサンプルをビルドし，実機で試してください．カーネルへ変更を加える際は，作業用ブランチで進め，変更前後の動作を比較してください．

## 目的に応じて探す

順番に学ばず，特定の機能だけを調べたい場合は[目的別索引](TASKS.md)を利用してください．コマンドの操作は[操作ガイド](guides/README.md)，APIと制約は[技術リファレンス](reference/README.md)，プログラムやOSの改造は[開発ガイド](development/README.md)にまとめています．

---

[文書案内](../DOCS.ja.md) ｜ [English](../en/ROADMAP.md)
