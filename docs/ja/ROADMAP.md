# PicoNIX学習ロードマップ

UNIX・OSの既習知識をPicoNIXの**操作 → 動作観察 → ソースコード読解 → 改造**につなげる道案内です．詳細は各ガイドやリファレンスを参照してください．

はじめて利用する場合は[セットアップ手順](setup/SETUP.md)で開発環境を整えてください．
## 対象読者と前提知識

この教材は，**UNIX/Linuxの基本操作，OSの基礎（プロセス・システムコール・FD・仮想記憶），C言語のポインタ・構造体を既に学んだ学生**を対象とします．TCP/IPの基本知識はPico Wのネットワーク編で，ARM/Thumbと例外処理の知識はカーネルの低レベル部分で用います．これらを最初から教える入門書ではありません．

学ぶ中心は，既習の概念が**RP2040上の小さな実装でどのように成立するか**です．特に固定長ユーザスロット，Flash上のカーネルとRAM上のユーザプログラム，PIC，SVCとPendSV，VFS，lwIPとの接続を，実行結果とソースコードで対応付けます．Linuxと同じ名前の機能でも，権限保護や仮想記憶などは異なるため，[Linuxとの差異](reference/DIFFERENCES.md)を確認してください．

**推奨する進み方：** 操作ガイドで現象を観察 → リファレンスでPicoNIX固有の仕様を確認 → 開発ガイドで対応する関数を読む → 必要なら別の作業ブランチで変更・検証，という順序です．
各項目は，上から順に進めることを推奨します．操作の詳細やトラブル対応はリンク先を参照してください．Pico Wが必要な項目は「選択」と明記しています．

## 1．まず使ってみる

| 順序 | 学ぶこと・試すこと | 読む資料 |
|---|---|---|
| 1 | PicoNIXの導入とUSB端末接続 | [セットアップ](setup/SETUP.md)・[helpと基本操作](reference/commands/HELP.md) |
| 2 | ファイルやディレクトリを調べる | [ファイルシステム](reference/FILESYSTEM.md)・[ファイルの編集](reference/commands/EDIT.md) |
| 3 | コマンドの出力をつなぐ | [パイプとFD](reference/commands/PIPE.md) |
| 4 | プロセスと端末を扱う | [TTY・stty](reference/commands/TTY.md)・[端末画面](reference/commands/TERMINAL.md) |

**確認してみること：** `help`でコマンドを調べ，`ls /bin`で実行できるプログラムを探し，`echo hello | cat`を実行してください．入出力がどのようにつながるか説明してみましょう．

## 2．OSの内部を観察する

| 順序 | 学ぶこと・試すこと | 読む資料 |
|---|---|---|
| 5 | ファイルとFD，VFSの関係 | [ファイルシステム](reference/FILESYSTEM.md)・[パイプ](reference/commands/PIPE.md) |
| 6 | メモリとユーザプロセスの配置 | [mem操作ガイド](reference/commands/MEM.md)・[SRAM配置](reference/MEMORY-LAYOUT.md) |
| 7 | システムコールを追跡する | [trace操作ガイド](reference/commands/TRACE.md)・[システムコール一覧](reference/SYSCALLS.md) |
| 8 | GPIOなどのデバイス操作 | [デバイス仕様](reference/devices/README.md)・[ピン割当](reference/hardware/PINS.md)・[配線](reference/hardware/WIRING.md) |
| 9（選択） | Pico Wの通信を観察する | [ネットワーク接続](reference/network/NETWORK.md)・[ip](reference/network/IP.md)・[ping](reference/network/PING.md)・[tcpdump](reference/network/TCPDUMP.md) |

**確認してみること：** `trace run hello`でシステムコールを観察し，[traceの説明](reference/commands/TRACE.md)に従って`ENTER`と`EXIT`の意味を確認してください．`mem`は[専用ガイド](reference/commands/MEM.md)を読んでから操作してください．

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

順番に学ばず，特定の機能だけを調べたい場合は[目的別索引](TASKS.md)を利用してください．コマンドの操作は[コマンドリファレンス](reference/commands/README.md)，APIと制約は[技術リファレンス](reference/README.md)，プログラムやOSの改造は[開発ガイド](development/README.md)にまとめています．

---

[文書案内](README.md) ｜ [English](../en/ROADMAP.md)

