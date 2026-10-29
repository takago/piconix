# 端末設定（`stty`）とゲーム（`mb`）

[概要](README-JP.md) | [UART1と端末](UART-JP.md) | [セットアップ](SETUP-JP.md)

PicoNIXには最小限の端末設定機能があります．POSIX `termios` の完全実装ではありません．

## `stty` で入力モードを確認・変更する

```text
stty
stty -icanon -echo
stty icanon echo
stty raw
stty cooked
```

- `icanon`：行単位の入力，`-icanon`：文字単位の入力
- `echo`：入力を表示，`-echo`：入力の表示を停止
- `raw`：`-icanon -echo` の別名，`cooked`：`icanon echo` の別名

`VMIN`／`VTIME`には対応していません．`stty raw`をシェルから実行すると行入力が難しくなるため，通常はアプリ内で一時的に切り替えてください．

## ノンブロッキング入力

`fcntl(fd, F_GETFL, 0)`と`fcntl(fd, F_SETFL, flags)`をサポートします．コンソール・UART1のFDでは`O_NONBLOCK`を変更できます．FDフラグは端末モードとは異なり，各タスクのFDに属します．

端末モードの変更は`ioctl()`で行います．モードは端末で共有するため，設定を変更したタスクが終了すると，カーネルは標準の`icanon echo`へ戻します（変更前の任意の値に戻す仕組みではありません）．

## `mb`（moonbuggy）

```text
mb
```

SPACEでジャンプ，`q`で終了します．ゲームは起動時の端末設定と標準入力FDフラグを保存し，文字単位・非エコー・ノンブロッキング入力に切り替えます．通常終了時には保存値を復元します．入力取得にはノンブロッキング`read()`と`msleep()`を使用します．

## `reset`による画面初期化

```text
reset
```

USB CDCコンソールの`icanon`・`echo`を有効にし，ANSI端末の表示状態を初期化します．`O_NONBLOCK`は変更しません．行入力そのものができない状態では，このコマンドを入力して復旧することはできません．UART1では使用できません．

端末の所属，`getty`，Ctrl+C／Ctrl+Dについては[UART1と端末操作](UART-JP.md)を参照してください．
