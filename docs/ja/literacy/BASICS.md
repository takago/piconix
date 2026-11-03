# 1．UNIXの基本操作

[学習ガイド](LEARNING-PATH.md) | [English](../../en/literacy/BASICS.md) | [コマンド一覧](HELP.md)

## 目的と準備

**目的：** コマンド，シェル，パス，ディレクトリの関係を説明し，自分で情報を探せるようになることです．[セットアップ](../setup/SETUP.md)を済ませたPicoまたはPico WをUSBで接続します．追加の回路は不要です．以下の`piconix#`はPicoNIX側の入力です．

## 実習

```text
piconix# help
piconix# pwd
piconix# ls /
piconix# ls /bin
piconix# ls /dev
piconix# ls /proc
piconix# ls /tmp
```

`/bin`にはコマンド，`/dev`にはデバイスへの入口，`/proc`には動作中の情報が現れます．`/tmp`はRAM上の書き込み可能な領域です．すべてのパスが同じ性質の保存ファイルではありません．

## 観察・考察

- `pwd`の結果は`ls /`の結果とどう違いますか．
- `/dev`と`/proc`の内容は，なぜ通常の保存ファイルとは違うのでしょうか．
- `help`で調べられる操作と，Linux PCのコマンドを混同していないか確認してください．

## 発展

[ファイルと標準入出力](IO-IPC.md)へ進み，表示したコマンドの出力をファイルに保存してみましょう．
