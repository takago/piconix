# PicoNIX 学習ガイド — 10のテーマ

[English](../../en/literacy/LEARNING-PATH.md) | [文書一覧](../../DOCS.ja.md) | [セットアップ](../setup/SETUP.md)

このガイドは，工学部の学部生が**操作から観察，プログラミング，OSの拡張へ**進むための道筋です．最初からOS内部の知識は不要です．Linux PCで行うビルドとPicoNIX端末で行う操作は区別します．例の`piconix#`はPicoNIXのプロンプトです．

**準備：** Raspberry Pi PicoまたはPico W，データ通信可能なUSBケーブル，ビルド用Linux PC．[Setup](../setup/SETUP.md)で書き込み・端末接続を済ませてください．テーマ8のネットワークにはPico Wが必要です．テーマ3の外部回路を使う部分は配線が確認できてから実施します．`/tmp`はRAM上なので再起動で消えます．

## 学習テーマと順序

| 順序 | 学習テーマ | 最初の教材 | 到達目標 |
|---|---|---|---|
| 1 | UNIXの基本操作 | [基本操作](BASICS.md) | シェル，パス，コマンドを説明できる |
| 2 | 標準入出力とプロセス間通信 | [標準I/O・パイプ](IO-IPC.md) | FD，リダイレクト，パイプを区別できる |
| 3 | デバイスと入出力 | [デバイスI/O](DEVICE-IO.md) | `/dev`と入出力APIの役割を説明できる |
| 4 | プロセスとタスク | [プロセス](PROCESSES.md) | PID，前景・背景実行を説明できる |
| 5 | メモリとプログラムの実行 | [メモリと実行](MEMORY-EXECUTION.md) | RAM，Flash，PICとスロットの関係を理解する |
| 6 | 端末と対話処理 | [端末実習](TERMINALS-LAB.md) | 端末設定と画面制御の違いを説明できる |
| 7 | システムモニタリング | [モニタリング実習](MONITORING.md) | `ps`，`mem`，`trace`などでOSを観察できる |
| 8 | ネットワーク | [ネットワーク実習](NETWORKING.md) | IP，TCP，Telnet，HTTPを観察できる（Pico W） |
| 9 | プログラミング | [言語別ガイド](#9プログラミングの選択) | C・Python・アセンブリの違いを理解する |
| 10 | OSの拡張 | [拡張ガイド](#10osの拡張) | システムコール，ドライバ，カーネルの改造方法を理解する |

すべてを一度に終える必要はありません．テーマ1～7は通常のPicoでも進められます．テーマ8はPico Wを使用する場合に選択します．**テーマ9・10は興味と前提知識に合わせた発展学習**で，すべての言語・改造方法を履修する必要はありません．

## 最初の15分の確認

```text
piconix# help
piconix# pwd
piconix# ls /bin
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# ps
```

この実習で，どの操作がファイルに作用し，どの操作が現在のOSの状態を示したかを説明してみてください．

## 9．プログラミングの選択

| 言語 | 最初に読む文書 | 作業場所 |
|---|---|---|
| Python風インタプリタ | [Python](../development/apps/python/PYTHON.md) | PicoNIXで対話入力・小さなスクリプト |
| C | [Cアプリ開発](../development/apps/c/PROGRAMMING.md) | Linux PCでビルドしPicoNIXで実行 |
| Thumbアセンブリ | [アセンブラ](../development/apps/asm/ASM.md) | PicoNIXの小型`as`で試す |

Python風インタプリタはCPythonではありません．Cアプリの追加にはファームウェア再ビルドが必要です．アセンブリはメモリ保護のない環境で実行されるため小さい例から始めます．

## 10．OSの拡張

[システムコールの追加](../development/syscalls/SYSCALL-DEVELOPMENT.md) → [ドライバの追加](../development/drivers/DRIVER-DEVELOPMENT.md) → [カーネル開発](../development/kernel/KERNEL-DEVELOPMENT.md)の順に学べます．改造前に[システムコール仕様](../reference/SYSCALLS.md)と[メモリ配置](../reference/MEMORY-LAYOUT.md)を確認してください．

## 観察・考察を記録する

各テーマで「何を実行したか」「どの出力を見たか」「その結果から何が分かるか」「予想と違った点」を数行で記録します．値やPIDは実行のたびに変わり得るため，サンプルと数値が一致しなくても直ちに失敗ではありません．

## 用語の手引き

| 用語 | 意味 |
|---|---|
| シェル | コマンドを受け取りプログラムを実行する環境 |
| プロセス／タスク | 実行中のプログラム．PicoNIXは固定RAMスロットで管理 |
| FD | 開いているファイルや端末を指す番号．0／1／2は標準入力／出力／エラー |
| VFS | ファイルやデバイスを共通のパス・操作で扱う仕組み |
| システムコール | ユーザプログラムがカーネルの機能を呼び出す入口 |
| PIC | 配置アドレスに依存しないコード |
| XIP | Flash上のコードをRAMへ全コピーせずに実行する方式 |

**参照先：** 各機能の正確な仕様は[Reference](../reference/SPECIFICATION.md)，デバイスと配線は[デバイスガイド](../reference/devices/DEVICES.md)と[ハードウェア仕様](../reference/hardware/HARDWARE.md)を参照してください．
