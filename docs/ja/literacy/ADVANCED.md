# 上級コース — ソースを読み，OSを拡張する

[学習コース一覧](LEARNING-PATH.md) | [中級へ](INTERMEDIATE.md) | [English](../../en/literacy/ADVANCED.md)

**到達目標：** 小さなCプログラムがシステムコールを通じてカーネルへ処理を依頼する経路を追跡し，カーネル内の担当機能と安全な拡張手順を説明できることです．最後の改造課題は任意です．

**前提：** [中級コース](INTERMEDIATE.md)相当の知識，Cの関数・ポインタ・構造体，Linux PCでのビルド操作．Cortex-M0+の例外処理とThumbアセンブリは必要になったところで学びます．作業前に[全体構造](../reference/ARCHITECTURE.md)，[メモリ配置](../reference/MEMORY-LAYOUT.md)，[Linuxとの違い](../reference/DIFFERENCES.md)を確認してください．

## 1．ユーザプログラムとCライブラリ

[Cアプリ開発](../development/apps/c/PROGRAMMING.md)で最小のプログラムから始め，[Cライブラリ](../development/apps/c/LIBC.md)で関数の提供範囲を調べます．Linux PCで次のファイルを開いてください．

```text
userland/apps/hello.c
userland/lib/syscalls.c
include/piconix_syscall.h
```

**調べること：** `hello`の出力が`write()`に達するまでを追います．PicoNIXのユーザプログラムはFlash上のPICイメージとして格納され，起動時にRAMスロットで実行されます．Cアプリを追加する場合はファームウェアの再ビルドが必要です．

**選択学習：** 短いスクリプトを実機で書くなら[Python](../development/apps/python/PYTHON.md)，Thumb命令を直接体験するなら[アセンブラ](../development/apps/asm/ASM.md)があります．どちらも必修ではありません．

## 2．システムコールの入口を追う

PicoNIX上で`trace run hello`を実行し，[システムコール一覧](../reference/SYSCALLS.md)と対照します．ソースは次の順で読みます．

```text
userland/apps/hello.c
  → userland/lib/syscalls.c
  → include/piconix_syscall.h
  → kernel/context.S（SVC）
  → kernel/kernel.c（os_svc_dispatch()）
```

**調べること：** システムコール番号と引数はどこで指定され，カーネルが戻り値を返す場所はどこでしょうか．アプリからカーネルへの入口と，タスクの切替処理を区別してください．

## 3．カーネルリーディング

[カーネルリーディングガイド](../development/kernel/KERNEL-READING-GUIDE.md)に沿って，次の順番で関数を追います．個々の定義箇所は[関数一覧](../../FUNCTIONS.md)を索引にしてください．

1. 起動とタスク生成：`kernel/main.c`，`kernel/kernel.c`
2. スケジューラとコンテキストスイッチ：`kernel/kernel.c`，`kernel/context.S`
3. システムコールとVFS・FD：`kernel/context.S`，`kernel/kernel.c`
4. TTYとパイプ：`kernel/kernel.c`，`userland/apps/shell.c`
5. **デバイス**：`kernel/gpio.c`，`fb.c`，`adc.c`，`i2c.c`，`pwm.c`
6. **ネットワーク（Pico Wのみ・任意）**：`kernel/net.c`，`kernel/net_socket.c`

**調べること：** 一つの`write()`がユーザコードから端末へ届くまでを，関数名で図示してください．この段階ではコードを変更する必要はありません．

## 4．小さな拡張を設計する（任意）

[カーネル拡張ガイド](../development/kernel/KERNEL-DEVELOPMENT.md)には，`getuptime()`と`/dev/uptime`を題材にした実習があります．[システムコール追加](../development/syscalls/SYSCALL-DEVELOPMENT.md)と[ドライバ追加](../development/drivers/DRIVER-DEVELOPMENT.md)を読み，いきなり実装する前に以下を整理してください．

- カーネル側で変更する関数と，共通ヘッダ・ユーザ側ラッパの役割
- `/dev`とシステムコールのどちらを入口にするか，その理由
- 入力値の検査，メモリ容量，既存コマンドへの影響
- ビルド後の確認方法と，関連するリファレンスの更新箇所

変更した場合は[セットアップのビルド手順](../setup/SETUP.md)と既存のテストに従い，実機で確認します．これはハードウェアに依存するOSなので，ソースを読んだだけで動作保証はできません．

## 5．ネットワークアプリを学ぶ（Pico Wのみ・任意）

**デバイスの仕組みを学んだ後で**，[ネットワークプログラミング](../development/apps/c/NETWORK-PROGRAMMING.md)に進みます．`userland/apps/httpd.c`と`kernel/net_socket.c`を比べ，`socket()`，`bind()`，`listen()`，`accept()`がどこで扱われるかを調べてください．Wi-Fi初期化と，ユーザアプリケーションのTCP/UDPソケットを別々の役割として整理します．

## 到達確認

1. `hello`の起動から終了までの主要なカーネル処理を説明できる．
2. `write()`，SVC，VFS，TTYの呼び出し関係を示せる．
3. ユーザアプリの変更とカーネルの変更で，ビルド・検証の範囲が違うことを理解している．
4. デバイス・ネットワークを別の機能群として把握し，それぞれの責務を説明できる．

**次の資料：** 実装の仕様は[システムコール](../reference/SYSCALLS.md)，[内部構造](../reference/INTERNAL.md)，[関数一覧](../../FUNCTIONS.md)で確認します．設計の歴史を知る場合は[開発日誌](../../DEVELOPMENT_DIARY.ja.md)を参照してください．
