# デバイスノードとドライバ開発：`/dev/uptime`を作る

この実習では，**配線不要の仮想デバイス**`/dev/uptime`を新設します．パス名，内部デバイスID，open時のアクセス制御，FD位置，デバイス固有のread処理を**一通り自分で実装**するための題材です．外部GPIOを不用意に操作しないので，通常のPicoでも試せます．システムコールの追加実習と組み合わせると，同じ起動時間が異なる経路で提供される様子を比較できます．

## 1．何を実装するのか

```text
cat /dev/uptime
  → open("/dev/uptime", O_RDONLY) → vfs_fixed_path_id() → DEV_UPTIME
  → read(fd,buf,n) → vfs_rw() → uptime_device_read()
  → 「12.345 s\n」などのテキスト → 標準出力
```

既存の`/proc/uptime`と機能的に重複しますが，ここでは**ドライバを独立したCファイルとして作る練習**が目的です．デバイスノードは物理部品を必要としません．パスがVFSから参照できることと，実際に`read()`が働くことは別々の実装段階です．

**先に確認：** [ファイルシステム](../../reference/FILESYSTEM.md)，[デバイス仕様](../../reference/devices/README.md)，[システムコール演習](../syscalls/SYSCALL-DEVELOPMENT.md)．ユーザアプリに使う`read()`や`open()`の使い方は[C開発](../apps/c/PROGRAMMING.md)を参照してください．

## 2．デバイスIDとパスを登録する

`include/device_names.h`のenumへ**265**を追加します．現行ソースの`200～263`はユーザシンボル領域で，パイプ内部IDは268以降です．265を他用途で使用している最新版にはそのまま適用しないでください．

```c
    DEV_NET_ICMP = 145,
    DEV_UPTIME = 265,
```

同じファイルの`PICONIX_FIXED_DEVICES(X)`末尾を変更します．前の行にはバックスラッシュが必要です．

```c
    X("/dev/i2c/1", DEV_I2C1) \
    X("/dev/uptime", DEV_UPTIME)
```

`kernel/kernel.c`の`dev_registry[]`はこのマクロで作られるため，`ls /dev`や`/proc/devices`にも反映されます．**この段階ではread処理はまだありません**．

## 3．独立したドライバのヘッダとソースを作る

新しい`kernel/uptime_device.h`を作成します．

```c
/* Simple read-only virtual device used by the documentation exercise. */
#ifndef PICONIX_UPTIME_DEVICE_H
#define PICONIX_UPTIME_DEVICE_H
#include <stdint.h>
#include <stddef.h>
int uptime_device_read(void *dst, size_t capacity, uint32_t *position, uint32_t ticks_ms);
#endif
```

続いて`kernel/uptime_device.c`を作ります．`ticks_ms`を文字列に直し，FDごとの`position`に従って小分けに返します．`*position`が末尾に達したら0を返すことが**EOFの条件**です．

```c
/* Read-only virtual driver.  No hardware pins are used. */
#include "uptime_device.h"
#include "../include/piconix.h"
#include <stdio.h>
#include <string.h>

int uptime_device_read(void *dst, size_t capacity, uint32_t *position, uint32_t ticks_ms) {
    char text[32];
    int len = snprintf(text, sizeof text, "%lu.%03lu s\n",
                       (unsigned long)(ticks_ms / 1000u),
                       (unsigned long)(ticks_ms % 1000u));
    if (len < 0 || (size_t)len >= sizeof text) return OS_EINVAL;
    if (*position >= (uint32_t)len || capacity == 0) return 0;
    size_t count = (size_t)len - *position;
    if (count > capacity) count = capacity;
    memcpy(dst, text + *position, count);
    *position += (uint32_t)count;
    return (int)count;
}
```

`read()`が例えば8バイトしか要求しなくても，`position`を進めて次の`read()`に続けられます．`n==0`では0バイトを返します．現在のドライバは**readのたびに時刻を文字列へ変換するため，分割read中に時間が変化すると文字列が食い違う可能性があります**．厳密なスナップショットにするならopen時に値を保存する設計が必要です．

## 4．CMakeへ組み込む

`CMakeLists.txt`のカーネル用`add_executable(piconix ...)`のファイル列へ`kernel/uptime_device.c`を追加します．**アプリ用`foreach(APP IN ITEMS ...)`とは別**です．

```cmake
add_executable(piconix ... kernel/kernel.c kernel/uptime_device.c kernel/gpio.c ...)
```

上の行は変更位置を示す**抜粋**であり，`...`を実際のCMakeへ貼り付けてはいけません．既存の`add_executable`のリストにファイル名だけ追記してください．

## 5．VFSの3か所を結ぶ

`kernel/kernel.c`のinclude群へ追加します．

```c
#include "uptime_device.h"
```

次に`vfs_open()`の**読み取り専用デバイスを検査する条件**へ`dev==DEV_UPTIME`を追加します．次は既存条件の末尾の抜粋です．

```c
... || dev==DEV_TRACE || dev==DEV_UPTIME) &&
    (flags & 3)!=OS_O_RDONLY)) return OS_EINVAL;
```

`vfs_rw()`では`DEV_RAND`などの分岐の直後，未対応IDのフォールバックより前へ追加します．

```c
if (dev == DEV_UPTIME) {
    if (writing) return OS_EBADF;
    return uptime_device_read(p, n, &tasks[current].fd_pos[fd], ticks);
}
```

`vfs_rw()`の共通入口にはユーザバッファの範囲検査があります．ドライバはそれに加えてバッファ長・読み取り位置・EOFを処理します．ここまで追加して初めて`cat /dev/uptime`が成立します．`ioctl`は今回は不要です．

## 6．実行・検証


```text
piconix# ls /dev
... uptime ...
piconix# cat /proc/devices
... uptime ...
piconix# cat /dev/uptime
12.345 s
piconix# labuptime
getuptime: 12399 ms
12.399 s
```

上記は**想定表示**であり，実測ログではありません．数値が一致しないのは呼び出し時間が異なるからです．`echo 1 > /dev/uptime`のように書き込みで開こうとすると失敗することも確認してください．`cat /dev/uptime`が空や無限表示なら，FD位置更新，0バイトでのEOF，VFSの振り分けを調べます．

**確認項目**：`open`の読み取り専用制限／`read`の短いバッファ・EOF／複数回open/close／未知パスの拒否／`ls /dev`での列挙．PicoとPico Wでビルドし，他のデバイス機能に影響がないかも確認します．

## 7．物理デバイスへ発展させるには

この演習は**仮想ドライバ**です．外部LEDやI²Cセンサーなどの実デバイスへ進むときは次の実装を照合します．

| 読む対象 | 調べるポイント |
|---|---|
| `kernel/gpio.c` | `gpio_driver_rw()`：値，方向，プルアップ，読書きの判定 |
| `kernel/pins.h` | ピンの予約と利用可能性の制限 |
| `kernel/i2c.c` | I²C1のトランザクションと制御要求 |
| `kernel/kernel.c` | FDから各ドライバへ処理を渡す分岐 |
| `reference/hardware/PINS.md` | GP番号と実際の配線の対応 |

例えば既存のGPIOをPicoNIXで操作するには`gpio`コマンドまたは`/dev/gpio/N`を使い，それらが`kernel/gpio.c`の`gpio_driver_rw()`を通ることを追ってください．物理ピンの割当は[ピン配置](../../reference/hardware/PINS.md)と[配線](../../reference/hardware/WIRING.md)を必ず参照し，LEDマトリクスやUART，I²C，PWMが使う予約ピンを別の周辺機器へ流用しないでください．**新規の物理ドライバを追加する場合は安全なGPIO所有権・初期化・競合管理・電気的制限も必要**であり，この仮想ドライバのコードをそのままハードウェア制御に流用しないでください．

## 8．実験後に確認する文書


**検証範囲：** ドライバ単体のC構文は確認済みですが，SDKでの完全ビルド・実機の読み書きは未検証です．

---

[開発ガイド一覧](../README.md) ｜ [システムコール演習](../syscalls/SYSCALL-DEVELOPMENT.md) ｜ [English](../../../en/development/drivers/DRIVER-DEVELOPMENT.md)
