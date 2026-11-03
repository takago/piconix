# 6．端末と対話処理

[学習ガイド](LEARNING-PATH.md) | [English](../../en/literacy/TERMINALS-LAB.md) | [端末設定の仕様](TTY.md)

## 目的と準備

**目的：** 端末の行入力，エコー，カーソル表示と，ファイルディスクリプタの読み取り方式を区別することです．USB端末で実習できます．端末が乱れたときに戻すため，まず[端末の使い方](TERMINAL.md)を確認してください．

## 実習

```text
piconix# tty
piconix# stty
piconix# stty raw
piconix# stty
piconix# stty cooked
piconix# stty
```

`raw`と`cooked`はPicoNIX独自の簡易設定であり，POSIXの完全な`termios`モードではありません．特に`raw`は簡易的にcanonical入力とエコーを無効化します．必要な作業を終えたら`stty cooked`へ戻してください．`stty`の設定と，アプリがANSIシーケンスで行う画面制御は別物です．

## 観察・考察

- キーを押すたびにアプリが入力を読む方式と，改行後に一行を受け取る方式はどう違いますか．
- `echo`設定とシェルによる入力文字の表示を混同していませんか．
- `edit`や`vi`が画面を描画できることと，カーネルがカーソルを管理することは同じですか．

## 発展

[TTYの仕様](TTY.md)で`ioctl()`と`fcntl()`の役割を比較し，[端末表示と復帰](TERMINAL.md)で代替画面と`reset`の扱いを調べましょう．
