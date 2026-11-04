# カーネル拡張ガイド

学習目的で最初から読む場合は[カーネル読解ガイド](KERNEL-READING-GUIDE.md)を利用してください．

## 読み始める前に

**事前学習：** [全体構成](../../reference/ARCHITECTURE.md)，[メモリ配置](../../reference/MEMORY-LAYOUT.md)，[トレース](../../literacy/TRACE.md)を先に確認してください．最初はソースを読み，タスクの生成から終了までの流れを追うことを勧めます．


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

以下の演習では両方の拡張方法を比較します．

## 4．実践演習：`getuptime()`と`/dev/uptime`を追加する

この演習では，**同じ起動後経過時間を，システムコールは数値で，デバイスノードは文字列で返す**実装を学びます．どちらもハードウェア追加は不要です．以下は**既存ソースに対する変更例**であり，完成済みパッチではありません．演習のコードは実機でのビルド・動作未検証です．

### 4.1 既存機能と設計上の注意

現行カーネルには1 ms周期で増える`ticks`があり，すでに`/proc/ticks`（ミリ秒の整数）と`/proc/uptime`（秒の文字列表現）が存在します．新設する`/dev/uptime`は機能的には重複しますが，**システムコールとVFSの拡張方法を比較する教材**として作ります．通常の実用アプリは既存の`/proc/uptime`を利用できます．

| インターフェース | 新設する動作 | 比較対象 |
|---|---|---|
| `unsigned getuptime(void)` | 起動後のミリ秒値を返す（32 bit） | `/proc/ticks` |
| `/dev/uptime` | `123.456 s\n`のようなテキストを返す | `/proc/uptime` |

`ticks`は`uint32_t`で，約49.7日後に周回します．32 bit値のビット列をそのまま返すため，Cラッパーは`unsigned`とします．新規システムコール番号は**40**，デバイス内部IDは既存IDと重複しない値として**265**を演習用に選びます．264はプログラム登録ID 64のシンボルファイルに割り当てられるため使用しません（パイプ内部IDは268以降）．実際の開発時には最新の定義を再確認してください．

### 4.2 システムコールを追加する

`include/piconix_syscall.h`の`PICONIX_SYSCALL_TABLE(X)`末尾へ，既存の39番（`SYS_SHUTDOWN_TCP`）を変えずに40番を追加します．

```c
    X(SYS_SHUTDOWN_TCP, 39, "shutdown") \
    X(SYS_GETUPTIME,    40, "getuptime")
```

`kernel/kernel.c`の`os_svc_dispatch()`にある`switch(num)`へ次の分岐を追加します．`ticks`は既存の1 msカウンタです．

```c
case SYS_GETUPTIME:
    result = (int)ticks;   /* 32-bit のビット列を r0 に返す */
    break;
```

`userland/lib/syscalls.c`には既存の`call()`を使うラッパーを追加します．`SYS_GETUPTIME`は同ファイルでインクルードしている共有番号ヘッダに定義されます．

```c
unsigned getuptime(void) {
    return (unsigned)call(SYS_GETUPTIME, 0, 0, 0);
}
```

`userland/lib/unistd.h`に宣言を追加します．

```c
unsigned getuptime(void);  /* ミリ秒，32-bit周回あり */
```

このAPIは引数なしです．正の値すべてが正常な計時値として使われるため，一般的な負のエラーコード判定は適用しません．呼び出しに失敗した場合の別のエラー通知機構は提供しない，という設計上の制限も明記してください．

### 4.3 読み取り専用`/dev/uptime`を追加する

ここでは文字列整形や読み取り位置の管理を新たに複製せず，既存の`device_rw_proc()`にある`DEV_PROC_UPTIME`処理を再利用します．次のように共有ヘッダと`kernel/kernel.c`を変更します．

1．`include/device_names.h`の固定デバイスID列挙に`DEV_UPTIME = 265`を追加します（ほかの識別子や範囲と重複させない）．

2．同じヘッダの`PICONIX_FIXED_DEVICES(X)`へ固定パスを追加します．`kernel/kernel.c`の`dev_registry[]`はこのマクロから生成されます．これにより`/dev`の列挙および`/proc/devices`の一覧にも反映されます．

```c
    X("/dev/i2c/1", DEV_I2C1) \
    X("/dev/uptime", DEV_UPTIME)
```

3．`vfs_open()`の読み取り専用検査条件に`dev==DEV_UPTIME`を加えます．これにより`open("/dev/uptime", O_WRONLY)`や`O_RDWR`を拒否します．単に`vfs_rw()`で書き込みを拒否するだけでなく，オープン時点で拒否します．

4．`vfs_rw()`のデバイス振り分け（GPIOフォールバックより前）に次を挿入します．

```c
if (dev == DEV_UPTIME)
    return device_rw_proc(fd, DEV_PROC_UPTIME, p, n, writing);
```

既存の`device_rw_proc()`は，読み取り時に`fd_pos[fd]`を進め，末尾で`0`（EOF）を返し，書き込み時には`OS_EBADF`を返します．これで複数回の小さな`read()`も扱えます．**`dev_registry[]`へ登録するだけでは読み書きは実装されません．**

### 4.4 ユーザプログラムで比較する

新しいCコマンドを登録する場合は[PROGRAMMING.md](../apps/c/PROGRAMMING.md)の手順を使います．まずシェルから以下を試します．

```text
cat /proc/ticks
cat /proc/uptime
cat /dev/uptime
```

次に，CプログラムでAPIとデバイスを比較します（`printf`等の公開APIは[LIBC.md](../apps/c/LIBC.md)を参照）．

```c
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

int main(void) {
    unsigned ms = getuptime();
    printf("uptime: %u ms\n", ms);
    int fd = open("/dev/uptime", O_RDONLY);
    if (fd < 0) return 1;
    char buf[32];
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, (unsigned)n);
    close(fd);
    return n < 0 ? 1 : 0;
}
```

2回の取得時刻は厳密には一致しません．`/dev/uptime`は各read時点のスナップショットを用いるため，**途中で時刻が進む場合に部分読み取りをまたいで一貫したスナップショットにはなりません**．簡易教材としては十分ですが，厳密な整合性が必要ならopen時のスナップショット管理を設計します．

### 4.5 検証チェックリスト

- 既存の1〜39番と予約11・15番を変更せず，新設40番だけが追加されている．
- `getuptime()`の値が`/proc/ticks`とおおむね一致し，時間の経過とともに増える．
- `cat /dev/uptime`が`/proc/uptime`と同形式を返す．
- `ls /dev`と`cat /proc/devices`で新しいノードを確認できる．
- `open()`の書き込みモードを拒否し，`read()`の0バイト・短いバッファ・EOF・複数回open/closeを確認する．
- `python3 tools/run_tests.py`と，ツールチェーン準備後の`python3 tools/run_tests.py --all`を実行し，Pico/Pico Wで実機検証する．
- `SYSCALLS.md`，`LIBC.md`，`DEVICES.md`（日英）に新APIとノードを追記し，番号40と内部ID265が既存の自動テストで排除されないか確認する．


ファームウェア全体のビルド環境は[SETUP.md](../../setup/SETUP.md)を参照してください．静的テストのPASSだけでは実機動作は保証されません．新しい周辺機器や割込み処理を追加する場合は，競合や長時間動作も別途検証します．

この演習では**時間計測の新機能を発明することではなく，既存のカーネルデータを2種類のインターフェースで公開する方法を学ぶこと**が目的です．

## 5．関連文書

- [SYSCALLS.md](../../reference/SYSCALLS.md)：ABI，引数，戻り値
- [LIBC.md](../apps/c/LIBC.md)：Cライブラリとラッパー
- [DEVICES.md](../../reference/devices/DEVICES.md)：デバイスノードと配線
- [INTERNAL.md](../../reference/INTERNAL.md)：タスク，VFS，カーネル内部
- [PROGRAMMING.md](../apps/c/PROGRAMMING.md)：ユーザ空間Cアプリ追加
- [TRACE.md](../../literacy/TRACE.md)：SVCとタスク切替の観察
