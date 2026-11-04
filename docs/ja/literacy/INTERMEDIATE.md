# 中級コース — OSの動作を調べる

[学習コース一覧](LEARNING-PATH.md) | [初級へ](BEGINNER.md) | [上級へ](ADVANCED.md) | [English](../../en/literacy/INTERMEDIATE.md)

**到達目標：** 目に見えるコマンドの動作を，ファイルディスクリプタ（FD），端末，タスク，メモリ，デバイスなどのOSの仕組みと結び付けて説明できることです．ソースコードの変更は必要ありません．

**前提：** [初級コース](BEGINNER.md)相当の操作．USB接続だけで1～5を実施できます．第6節のネットワークはPico Wを使う場合の選択実習です．

## 1．プロセスとFDの関係

[プロセス](PROCESSES.md)，[標準入出力](IO-IPC.md)，[パイプ](PIPE.md)をもう一度読み，`lsof`が表示するFDの意味を調べます．

```text
piconix# ps
piconix# lsof
piconix# echo hello | cat -n
```

**考えること：** 標準入力0・標準出力1・標準エラー2は，どのような対象を指す番号でしょうか．パイプでつながれたコマンドはなぜ別々のタスクとして動けるでしょうか．[ファイルシステム仕様](../reference/FILESYSTEM.md)と[FDの管理](../reference/INTERNAL.md)も必要に応じて参照してください．

## 2．端末入力と画面表示

[端末実習](TERMINALS-LAB.md)の順序に沿って，`tty`と`stty`を試します．

```text
piconix# tty
piconix# stty
```

`stty raw`と`stty cooked`の切替は[TTY設定](TTY.md)で動作を確認してから実施し，実験後に`stty cooked`へ戻します．端末の入力設定はカーネル側，ANSIエスケープシーケンスによる画面描画はアプリ側が担うことを[端末と画面制御](TERMINAL.md)で確かめます．

## 3．RAM，Flashとプログラムの配置

[メモリと実行](MEMORY-EXECUTION.md)を読み，次の操作を実施します．

```text
piconix# ps
piconix# mem -s
piconix# mem -s kernel
piconix# mem -s sh
```

`mem`の全画面表示は[操作ガイド](MEM.md)を確認してから試します．**アドレスの書き換えではなく観察にとどめてください．** [メモリ配置](../reference/MEMORY-LAYOUT.md)で，Flashに保存されたPICイメージがRAMのタスクスロットで動作する仕組みを確認します．

## 4．システムコールを観察する

[モニタリング実習](MONITORING.md)と[トレース](TRACE.md)を読んで実行します．

```text
piconix# trace run hello
piconix# cat /proc/uptime
```

**考えること：** `hello`の表示はユーザプログラムから直接USBへ書き込まれるのでしょうか．`write()`を経由してカーネルの処理が呼ばれる関係を説明します．トレースの時刻やPIDは実行のたびに変わります．

## 5．デバイスをファイルとして見る

[デバイスI/O実習](DEVICE-IO.md)の「実習A」は外付け回路なしで進められます．

```text
piconix# ls /dev
piconix# ls /dev/gpio
piconix# cat /proc/fb
```

`/tmp`と`/dev`は同じように名前で開けても，読み書きの対象が異なります．[デバイス仕様](../reference/devices/DEVICES.md)を調べ，`read()`・`write()`・`ioctl()`の役割を比較してください．LEDやセンサを操作するときは[配線](../reference/hardware/HARDWARE.md)を先に確認します．

## 6．ネットワークを観察する（Pico Wのみ・任意）

[ネットワーク実習](NETWORKING.md)を順に進め，Wi-Fi，Telnet，HTTPとユーザプロセスのソケットの関係を調べます．最初に[接続方法](../reference/devices/NETWORK.md)を確認してください．TelnetとUDPデバイス制御には認証・暗号化がないため，**信頼できる実験用ネットワークでのみ**使います．ネットワークを利用しなくても上級コースへ進めます．

## 到達確認

1. パイプ・通常ファイル・TTYをFDという共通の仕組みで説明できる．
2. `ps`，`mem`，`trace`がそれぞれ何を観察する道具かを説明できる．
3. Flash上の実行イメージとRAM上のタスクスロットを区別できる．
4. `/tmp`・`/dev`・`/proc`が異なる対象であっても共通のI/O APIを使える理由を説明できる．
5. デバイスとネットワークを混同せず，Pico Wの通信が追加機能であることを説明できる．

次は[上級コース](ADVANCED.md)です．OSの実装を読む前に，[全体構造](../reference/ARCHITECTURE.md)と[Linuxとの違い](../reference/DIFFERENCES.md)に目を通します．
