# 初級コース — PicoNIXを使ってみる

[学習コース一覧](LEARNING-PATH.md) | [中級へ](INTERMEDIATE.md) | [English](../../en/literacy/BEGINNER.md)

**到達目標：** PicoNIXのシェルでコマンドを探し，RAMファイルを作り，標準入出力をつなぎ，実行中のプロセスを調べられるようになることです．OSのソースコードを読む必要はありません．

**準備：** [セットアップ](../setup/SETUP.md)を終えたPicoまたはPico WとUSB端末．以下はすべてPicoNIX上で入力します．`/tmp`の内容は再起動すると消えます．

## 1．シェルとディレクトリを調べる

[基本操作](BASICS.md)と[helpの使い方](HELP.md)を読み，次の操作を試してください．

```text
piconix# help
piconix# pwd
piconix# ls /
piconix# ls /bin
piconix# ls /dev
piconix# ls /proc
piconix# ls /tmp
```

**考えること：** `/bin`はプログラム，`/dev`はデバイス，`/proc`は稼働中の情報をどう見せているでしょうか．見た目がファイルでも，すべてが同じ保存形式とは限りません．

## 2．ファイルを作り，表示する

```text
piconix# echo hello > /tmp/note.txt
piconix# cat /tmp/note.txt
piconix# cat -n /tmp/note.txt
piconix# cp /tmp/note.txt /tmp/copy.txt
piconix# cat /tmp/copy.txt
```

`>`は画面ではなくファイルへ出力します．`cat -n`が付けた行番号は，保存された内容には追加されません．編集を試す場合は[edit](EDIT.md)または[vi](VI.md)，ファイル一覧を対話的に見たい場合は[fd](FD.md)を参照してください．

**考えること：** `echo`，`cat`，`cp`の役割はどう違うでしょうか．`/tmp`のファイルが再起動後に残らない理由は何でしょうか．

## 3．標準入出力とパイプを試す

[標準入出力の実習](IO-IPC.md)を参考に，ファイルを経由する方法とパイプで直結する方法を比べます．

```text
piconix# echo hello | cat
piconix# echo hello | cat -n
piconix# echo hello | grep hello | cat
piconix# lsof
```

`|`の左右は別のコマンドです．PicoNIXでは最大3コマンドのパイプラインを扱えますが，Linuxのシェルと同じ機能がすべて使えるわけではありません．詳しくは[パイプ](PIPE.md)を参照してください．

**考えること：** `cat /tmp/note.txt`と`echo hello | cat`では，`cat`の入力元はどう違うでしょうか．標準入力・標準出力という言葉で説明してみてください．

## 4．実行中のプロセスを調べる

[プロセスの実習](PROCESSES.md)を読み，次を試します．

```text
piconix# ps
piconix# blink &
piconix# ps
piconix# cat /proc/tasks
```

`&`は背景実行です．動作確認後は`ps`に表示された**実際のPID**を調べて`kill PID`で終了してください．`PID`という文字列をそのまま入力するわけではありません．タスクスロットは有限なので，不要な背景プログラムを残さないようにします．

## 到達確認

1. `help`で初めて見るコマンドの使い方を調べられる．
2. `/tmp`に書き込み，`cat`で読み出し，`>`と`|`の違いを説明できる．
3. `ps`のPIDが`/bin`のプログラム名と同じ意味ではないことを説明できる．
4. `/dev`，`/proc`，`/tmp`の違いを例示できる．

ここまで説明できたら[中級コース](INTERMEDIATE.md)へ進み，FDやメモリの内部動作を調べます．もっとコマンドに慣れたい場合は，[dd](DD.md)，[Pong](PONG.md)，[Python](../development/apps/python/PYTHON.md)を選択しても構いません．
