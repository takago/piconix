# システムモニタリング実習

[English](../../en/literacy/MONITORING.md) | [学習ガイド](LEARNING-PATH.md) | [トレース詳細](TRACE.md) | [メモリビューア](MEM.md)

この実習では，**実行中のOSを外から観察する**方法を学びます．プロセス，ファイルディスクリプタ（FD），システムコール，メモリの関係を考えます．Raspberry Pi PicoでもPico Wでも，USB接続だけで前半を試せます．Linux PCで実行する操作と，PicoNIX上で実行する操作を区別してください．以下の`piconix#`はPicoNIXのプロンプトを示します．

## 1．プロセスを観察する

```text
piconix# ps
piconix# cat /proc/uptime
piconix# cat /proc/tasks
```

**確かめよう：** `ps`で表示されるPIDとコマンド名，`/proc`に公開された情報の対応を説明してください．`/proc`は通常の保存ファイルではなく，実行時の状態を参照する入口です．実行のたびに内容が変わることがあります．

## 2．FDと標準入出力を結び付ける

```text
piconix# lsof
piconix# echo hello | cat
piconix# lsof
```

**確かめよう：** FD 0，1，2の役割を復習してください．`lsof`は実行中のタスクが開いているFDを表示します．短命な`echo`や`cat`は`lsof`実行前に終了するため，一覧に現れなくても正常です．実行中のタスクを調べるには`ps`でPIDを確認して`lsof PID`を使えます．[パイプの実習](PIPE.md)も参照してください．

## 3．カーネルへの依頼を可視化する

```text
piconix# trace run hello
piconix# trace run ls
```

**確かめよう：** `ENTER`／`EXIT`はシステムコールの開始／終了，`SWITCH`はタスク切り替えを表します．実行結果の行数やイベント数は環境によって異なります．出力を比較し，なぜ`ls`と`hello`で呼び出しが異なるのか考えましょう．`trace run`は指定したプログラムの終了後にトレースを表示します．リングバッファが満杯になると古い記録は失われます．詳しくは[トレースの仕様](TRACE.md)を参照してください．

## 4．RAMとFlashの違いを見る

```text
piconix# mem -s
piconix# mem
```

`mem`の全画面表示では，`1`〜`6`でユーザスロットを選び，`k`でカーネルRAM，`K`でXIP Flashへ移動します．`q`で終了します．**確かめよう：** アドレス範囲と，データが変化する場所を比較してください．表示は同一瞬間の完全なスナップショットではありません．`mem`は読み取り専用です．詳細は[メモリ操作](MEM.md)と[メモリ配置](../reference/MEMORY-LAYOUT.md)を参照してください．

## 5．Pico Wではネットワーク資源も観察する（発展）

```text
piconix# net
piconix# net -r
piconix# cat /proc/net/memory
```

通常のPicoにはWi-Fi機能がありません．`net`の情報を解釈するにはPico Wのネットワーク設定が必要です．`net -r`のARP表に通信相手がないことは，ただちに障害を意味しません．`/proc/net/memory`はネットワークスタックの状態確認に使います．詳細は[ネットワーク実習](NETWORKING.md)を参照してください．

## 振り返り

- `ps`，`lsof`，`trace`，`mem`はそれぞれ**何を**観察する道具でしょうか．
- トレースに現れた`read()`／`write()`と，ファイルディスクリプタはどのように結び付くでしょうか．
- 実行中の状態を繰り返し読み取るとき，なぜ値が変わるのでしょうか．

次は[ネットワーク実習](NETWORKING.md)へ進むか，[アプリ開発](../development/apps/c/PROGRAMMING.md)で観察対象のプログラムを作ってみてください．
