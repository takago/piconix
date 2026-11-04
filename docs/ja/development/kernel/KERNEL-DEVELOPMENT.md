# カーネル拡張ガイド

*開発ガイド*

学習目的で最初から読む場合は[カーネル読解ガイド](KERNEL-READING-GUIDE.md)を利用してください．

## 読み始める前に

**事前学習：** [全体構成](../../reference/ARCHITECTURE.md)，[メモリ配置](../../reference/MEMORY-LAYOUT.md)，[トレース](../../guides/TRACE.md)を先に確認してください．最初はソースを読み，タスクの生成から終了までの流れを追うことを勧めます．


[文書案内](../../../DOCS.ja.md) | [内部構造](../../reference/INTERNAL.md) | [システムコール](../../reference/SYSCALLS.md) | [デバイス](../../reference/devices/DEVICES.md) | [English](../../../en/development/kernel/KERNEL-DEVELOPMENT.md)

この文書は，**PicoNIXのカーネルに機能を追加する開発者**向けです．ユーザ空間のCコマンドを追加する場合は[PROGRAMMING.md](../apps/c/PROGRAMMING.md)を参照してください．以下は添付された現行ソースの構造に基づく手順であり，完成済みパッチではありません．既存のABIや機器予約を破壊しないよう，変更箇所を一つずつ確認してください．

## 1．最初に読むソース

| ファイル | 役割 |
|---|---|
| `kernel/kernel.c` | タスク，SVCディスパッチ，FD，VFS，デバイスI/O，TTY |
| `kernel/main.c` | Flash内のユーザプログラムの登録，起動 |
| `kernel/context.S` | 例外・コンテキスト切替 |
| `userland/lib/syscalls.c` | C側のSVCラッパー，BSD風アダプター |
| `userland/lib/unistd.h` | 公開APIの宣言・定数 |
| `include/piconix.h` | カーネルとユーザ側が共有する型・制限値 |
| `kernel/gpio.c`, `kernel/adc.c`, `kernel/pwm.c`, `kernel/fb.c` | 機器別バックエンド |
| `kernel/pins.h`, `kernel/devices.h` | ピン用途・デバイス関連の定義 |
| `tools/run_tests.py`, `tools/test_scripts/` | 回帰テスト |

[ARCHITECTURE.md](../../reference/ARCHITECTURE.md)と[INTERNAL.md](../../reference/INTERNAL.md)でXIP，RAMスロット，SVC境界を先に理解してください．PicoNIXは一般的なLinuxカーネルではなく，単一アドレス空間を使用し，ユーザプログラムのポインタは明示的に検査する設計です．

## 2．拡張ガイド

- [システムコールの追加](../syscalls/SYSCALL-DEVELOPMENT.md)：ABI，ディスパッチ，ラッパー，テスト
- [デバイスドライバの追加](../drivers/DRIVER-DEVELOPMENT.md)：VFS登録，入出力，ioctl，テスト
- [ネットワーク機能の拡張（Pico W）](../network/NETWORK-DEVELOPMENT.md)：Wi-Fi，lwIP，ソケット，カーネル常駐サービス

以下の演習では両方の拡張方法を比較します．

## 3．実践演習：システムコールと独立した仮想デバイス

この実習では，**同じ起動後経過時間を2通りのインターフェースから取得**します．既存の`/proc/ticks`と`/proc/uptime`を変更せず，学習用に`getuptime()`と`/dev/uptime`を追加します．

| 何を追加するか | 詳細な手順（コード付き） | 最終確認 |
|---|---|---|
| `getuptime()`のSVC | [システムコール追加ガイド](../syscalls/SYSCALL-DEVELOPMENT.md) | `labuptime`と`trace run labuptime` |
| `/dev/uptime`と独立したドライバ | [デバイスノード・ドライバ追加ガイド](../drivers/DRIVER-DEVELOPMENT.md) | `ls /dev`，`cat /dev/uptime`，EOFの確認 |
| プログラム登録・UF2作成 | [Cアプリケーション開発](../apps/c/PROGRAMMING.md)・[セットアップ](../../setup/SETUP.md) | `ls /bin`と実機実行 |

2つの実装ガイドには，変更するファイル，ソースコード全文，ビルドコマンド，失敗時の確認方法，演習用の`docs/examples/uptime_lab.patch`を載せています．**サンプルパッチを適用するのは別の作業ブランチだけ**にしてください．ここで同じコードを重複掲載しないのは，将来の変更で説明が食い違うことを防ぐためです．

**課題：** カーネルの1 msカウンタから，SVC経由で数値を返す経路と，VFS/FD経由で文字列を返す経路をそれぞれ図示します．`/dev/uptime`はハードウェアを直接制御しない仮想デバイスなので，まずは配線不要でデバイスドライバの基本を学習できます．


## 4．関連文書

- [SYSCALLS.md](../../reference/SYSCALLS.md)：ABI，引数，戻り値
- [LIBC.md](../apps/c/LIBC.md)：Cライブラリとラッパー
- [DEVICES.md](../../reference/devices/DEVICES.md)：デバイスノードとAPI， [PINS.md](../../reference/hardware/PINS.md)：ピン割当， [WIRING.md](../../reference/hardware/WIRING.md)：物理配線
- [INTERNAL.md](../../reference/INTERNAL.md)：タスク，VFS，カーネル内部
- [PROGRAMMING.md](../apps/c/PROGRAMMING.md)：ユーザ空間Cアプリ追加
- [TRACE.md](../../guides/TRACE.md)：SVCとタスク切替の観察

---

**関連リンク：** [開発ガイド一覧](../README.md) ｜ [Part C 教科書](../../learn/C/README.md)
