# デバイスドライバ開発ガイド

*開発ガイド*

## 読み始める前に

**事前学習：** [デバイスの仕様](../../reference/devices/DEVICES.md)で`/dev`の意味を確認し，[Cアプリ開発](../apps/c/PROGRAMMING.md)で`open()`と`read()`に触れてから進んでください．この章のコード変更には再ビルドと実機検証が必要です．


[カーネル開発](../kernel/KERNEL-DEVELOPMENT.md) | [デバイスリファレンス](../../reference/devices/DEVICES.md) | [English](../../../en/development/drivers/DRIVER-DEVELOPMENT.md)

この文書はデバイスノードとカーネル側の処理の追加方法を説明します．既存デバイスの操作方法とは区別します．

## 1．新しいデバイスノードを追加する

### 1.1 既存のVFSを把握する

固定の`/dev`パスは`include/device_names.h`の`PICONIX_FIXED_DEVICES(X)`で定義され，`kernel/kernel.c`が`dev_registry[]`を生成します．`/proc`などの固定特殊パスはカーネル内の`vfs_registry[]`で管理されます．`vfs_open()`がパスを解決し，タスクのFDに内部デバイスIDを対応付けます．`vfs_rw()`が読み書きを振り分け，`device_ioctl()`が対応する制御要求を扱います．`close()`はFD解放を通じて資源を後始末します．

**パスを登録しただけでは`read()`や`write()`は動作しません．** 既存のGPIO，ADC，PWMは動的パス・条件付き公開を含むため，単純な固定ノードとは別に調べてください．`/proc/devices`や`/dev`ディレクトリ列挙にも反映されるか確認が必要です．

### 1.2 読み取り専用ノードを追加する場合

1. `DEV_*`識別子の未使用値を選び，既存のデバイスID・パイプ内部ID・ディレクトリID・プログラムシンボル用IDと重複しないことを確認します．
2. 固定ノードなら`include/device_names.h`の`PICONIX_FIXED_DEVICES(X)`に`"/dev/example"`と識別子を追加します．`/proc`の情報ノードなら名前・列挙箇所が異なるので既存の`vfs_registry[]`を参照してください．
3. `vfs_open()`で読み取り専用などのアクセスモード制限を追加します．現行の`OS_O_RDONLY`や`OS_O_NONBLOCK`制限を確認してください．
4. `vfs_rw()`で対象IDを分岐し，`writing`時は`OS_EBADF`，読み取り時は処理結果のバイト数，失敗時は負の`OS_E*`値を返します．呼出し元にはユーザバッファの範囲検査がありますが，**バックエンド固有の長さと状態**も別途検査します．
5. 継続読み取りが必要なら`tasks[current].fd_pos[fd]`を用いたオフセットとEOF（0バイト）を設計します．`lseek()`を必要とするならその対象FDの扱いも検討します．
6. `device_ioctl()`が必要なら要求番号と許可するFDを追加します．不要なら未対応要求は既存どおりエラーにします．
7. ピン・周辺機能を使う場合，`kernel/pins.h`・[ピン割当](../../reference/hardware/PINS.md)と既存ドライバの所有関係を確認し，USB／UART／Wi-Fi／FB／PWMの予約資源と衝突させないでください．
8. `DEVICES.md`，`FILESYSTEM.md`，必要なら`LIBC.md`を更新し，日英両方に同じパスと動作を記載します．

以下は利用側の**動作確認例**です．新ノードのカーネル実装そのものではありません．

```c
#include "unistd.h"
#include "fcntl.h"

int fd = open("/dev/example", O_RDONLY);
if (fd >= 0) {
    char buf[32];
    int n = read(fd, buf, sizeof buf);
    /* n > 0: データ，n == 0: EOF，n < 0: エラー */
    close(fd);
}
```

公開ヘッダの実際のマクロ名や引数型は変更時に照合してください．

### 1.3 デバイス追加時のテスト

| 観点 | 確認内容 |
|---|---|
| パス解決 | 正しいパスでopenでき，誤ったパスでは失敗する |
| 列挙 | `ls /dev`，必要なら`/proc/devices`から見える |
| 権限 | 読み取り専用FDでのwriteを拒否する |
| 入力検査 | 0バイト，境界値，不正ポインタを扱える |
| FD寿命 | 複数回open／close，タスク終了時に不整合がない |
| 排他 | 機器を同時利用するときの所有者・競合条件を定義する |
| ハードウェア | Pico/Pico Wの両方で未搭載機能を安全に扱う |

---

**関連リンク：** [開発ガイド一覧](../README.md) ｜ [Part C 教科書](../../learn/C/README.md)
