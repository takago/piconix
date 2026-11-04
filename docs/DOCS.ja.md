# PicoNIX ドキュメント案内

[プロジェクト概要](../README.md) | [English](DOCS.md) | [開発日誌](DEVELOPMENT_DIARY.ja.md)

文書は**学習コース**と**機能別の解説・リファレンス**に分けて利用します．初めて学ぶ場合は，項目を辞書的に探すのではなく，下のコースを順に進めてください．個別の技術文書は従来の場所に残しています．

## 体系的に学ぶ

**入口：[初級・中級・上級の学習コース一覧](ja/literacy/LEARNING-PATH.md)**

| 段階 | コース | 主な内容 | 進む目安 |
|---|---|---|---|
| **初級** | [使ってみる](ja/literacy/BEGINNER.md) | シェル，ファイル，パイプ，プロセス | コマンドを動かし，入出力を説明できる |
| **中級** | [動作を調べる](ja/literacy/INTERMEDIATE.md) | FD，端末，メモリ，トレース，デバイス，ネットワーク | OSの内部動作を観察して説明できる |
| **上級** | [コードを読む・拡張する](ja/literacy/ADVANCED.md) | C，SVC，VFS，カーネル読解，拡張 | コード中の呼出経路と役割を説明できる |

最初は[セットアップ](ja/setup/SETUP.md)を済ませます．通常のPicoでも大部分を学習できます．Pico Wを要する通信実習と外部回路を要する実習は任意です．**初級・中級ではコードを変更する必要はありません．**

## リファレンスと機能別ガイド

以降は，学習中に必要になったときに参照する資料です．「解説・実習」と「現在の仕様」を区別して使ってください．

### 導入と全体像

- [セットアップ](ja/setup/SETUP.md)：開発環境の導入，ビルド，書き込み
- [PicoNIXの概要・コマンド案内](ja/literacy/README.md)：できることと最初の操作
- [全体構造](ja/reference/ARCHITECTURE.md)：モジュールと実行モデル
- [Linuxとの違い](ja/reference/DIFFERENCES.md)：省略した機能と制限
- [システム仕様](ja/reference/SPECIFICATION.md)：現行の仕様
- [ハードウェア](ja/reference/hardware/HARDWARE.md)：配線と機材

### シェル・ファイル・端末

- [UNIX基本操作](ja/literacy/BASICS.md)，[help](ja/literacy/HELP.md)
- [標準入出力とIPC](ja/literacy/IO-IPC.md)，[パイプ詳細](ja/literacy/PIPE.md)
- [ファイルシステム](ja/reference/FILESYSTEM.md)，[初期ファイルと /etc/rc](ja/reference/INITIAL-FILES.md)
- [テキスト編集](ja/literacy/EDIT.md)，[vi](ja/literacy/VI.md)，[fdファイルマネージャ](ja/literacy/FD.md)，[ddとバイナリ転送](ja/literacy/DD.md)
- [端末実習](ja/literacy/TERMINALS-LAB.md)，[端末と画面制御](ja/literacy/TERMINAL.md)，[TTY設定](ja/literacy/TTY.md)，[Pong](ja/literacy/PONG.md)

### タスク・メモリ・カーネルの観察

- [プロセス実習](ja/literacy/PROCESSES.md)，[メモリと実行](ja/literacy/MEMORY-EXECUTION.md)
- [システムモニタリング](ja/literacy/MONITORING.md)，[トレース](ja/literacy/TRACE.md)，[メモリビューア](ja/literacy/MEM.md)
- [内部構造](ja/reference/INTERNAL.md)，[メモリ配置](ja/reference/MEMORY-LAYOUT.md)，[システムコール一覧](ja/reference/SYSCALLS.md)
- [カーネルシンボル](ja/reference/KERNEL-SYMBOLS.md)，[memシンボルの見方](ja/reference/MEM-SYMBOLS.md)

### デバイス・ハードウェア

- [デバイスI/O実習](ja/literacy/DEVICE-IO.md)，[デバイス仕様](ja/reference/devices/DEVICES.md)
- [I²C1](ja/reference/devices/I2C.md)，[UART](ja/reference/devices/UART.md)
- [リモートデバイス制御](ja/reference/devices/REMOTE-DEVICE.md)，[リモートGPIO](ja/reference/devices/REMOTE_GPIO.md)，[UDPデバイス制御](ja/reference/devices/UDP-DEVICES.md)

### ネットワーク（Pico W）

- [ネットワーク実習](ja/literacy/NETWORKING.md)，[ネットワーク接続と仕様](ja/reference/devices/NETWORK.md)
- [Wi-Fiモード](ja/reference/devices/WIFI-MODES.md)，[ネットワークコマンド](ja/reference/devices/NET.md)
- [nc](ja/reference/devices/NC.md)，[HTTPサーバ](ja/reference/devices/HTTPD.md)

## プログラミング・カーネル開発

ここにあるのは主に**上級の実装教材**です．初めに[上級コース](ja/literacy/ADVANCED.md)を読むと，ソースを追う順序が分かります．

- **ユーザアプリ：** [Cアプリ開発](ja/development/apps/c/PROGRAMMING.md)，[Cライブラリ](ja/development/apps/c/LIBC.md)，[ネットワークプログラミング](ja/development/apps/c/NETWORK-PROGRAMMING.md)
- **実機でのプログラミング：** [Python風インタプリタ](ja/development/apps/python/PYTHON.md)，[Thumbアセンブラ](ja/development/apps/asm/ASM.md)
- **カーネルを読む：** [カーネルリーディングガイド](ja/development/kernel/KERNEL-READING-GUIDE.md) → [関数一覧](FUNCTIONS.md) → [内部構造](ja/reference/INTERNAL.md)
- **カーネルを拡張する：** [システムコール追加](ja/development/syscalls/SYSCALL-DEVELOPMENT.md)，[ドライバ追加](ja/development/drivers/DRIVER-DEVELOPMENT.md)，[カーネル開発](ja/development/kernel/KERNEL-DEVELOPMENT.md)

## 歴史と開発記録

[開発日誌](DEVELOPMENT_DIARY.ja.md)は設計判断の経緯を説明します．[過去の記録](history/README.md)には現行仕様ではない検討・実装履歴も含みます．**現行のAPIや制約は歴史文書ではなくリファレンスを優先**してください．
