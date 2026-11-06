# カーネルトレース演習

この演習集は[トレース操作ガイド](../guides/TRACE.md)を読んだ後に使用します．コマンド仕様と記録形式はガイドを参照してください．

## SVC内と非同期処理の違いを観察する

この実習は，標準ビルドで有効になっている関数計測（`PICONIX_KERNEL_FUNCTION_TRACE=ON`）を想定します．イベント記録は起動時OFFで，`trace on`や`trace run`で開始します．明示的に`OFF`でビルドした場合は`KERNEL`行が表示されません．以下の出力は説明用であり，実行時刻，PID，関数の並びは環境によって異なります．

### 実習1：プログラムのシステムコールとカーネル関数

```text
piconix# trace run -s hello
piconix# trace run hello
piconix# trace run ls
piconix# trace run python test.py
```

`trace run -s hello`でSYSCALLだけの記録を観察し，通常の`trace run hello`と比較します．`-s`ではTIMER・SWITCH・KERNELは記録されません．次に`SYSCALL`行と`KERNEL`行を比べ，システムコールからVFSやデバイスドライバの選抜関数に処理が移る様子を探します．入口は関数名，出口は末尾に`/`を付けて表示されます．直ちに戻る関数は`(関数名)`と1行で表示される場合があります．関数イベントは通常モードではSVC処理中に限られます．`trace run`の表示は起動したPIDのイベントに絞られます．

### 実習2：pingによるICMPデバイス操作

```text
piconix# trace run ping 127.0.0.1
```

`/dev/net/icmp`に関係する`open`，`write`，`read`などのシステムコールを観察します．関数計測ビルドでは，SVC中に呼ばれる`net_echo_write`，`net_echo_read`，`net_echo_close`も記録候補になります．これらは呼び出された場合に限り表示されます．ICMPの非同期受信処理そのものをすべて追跡する機能ではありません．

### 実習3：別端末で非同期処理も観察する

USB CDCとUART1またはTelnetなど，**2つの端末**で試します．

端末A：

```text
piconix# trace on -a
piconix# trace follow
```

端末B：

```text
piconix# ping 127.0.0.1
piconix# ls
```

端末Aで`SYSCALL`，`SWITCH`，`TIMER`，`KERNEL`の各行を探します．`-a`は**選抜されたカーネル関数**の記録条件をSVC外にも広げます．非同期処理や割込み中の関数イベントにはPIDを付けず`-`と表示します．ただし，対象として登録・計測されていない関数（`net_capture_poll`や`echo_received`など）は記録されません．SysTickの`TICKx1K`は関数イベントとは別の通常イベントなので，`-a`なしでも記録されます．

終了するときは，端末Bなど別端末から次を実行します．

```text
piconix# trace off
```

`trace follow`は残りのイベントを表示して終了します．`trace show`は履歴表示と同時に記録を停止する点にも注意してください．

### 実習4：`trace run -a`との違い

```text
piconix# trace run -a ping 127.0.0.1
```

この指定も全コンテキストの選抜関数を**記録**しますが，実行後の一覧は起動した`ping`のPIDだけを**表示**します．PIDなしの非同期関数イベントまで観察するには，実習3の`trace on -a`と`trace follow`を使います．記録量が増えると512件のリングバッファの古いイベントが上書きされるため，長時間の記録は避けてください．

## ファイル・デバイス・ネットワーク・スケジューリング

以下の実習は，単独で終了するプログラムには `trace run` を，サーバなど継続して動作するプログラムには別端末から `trace on` と `trace follow` を使います．`trace run` は対象プログラムが終了するまで結果を表示しません．記録を停止するときは別端末から `trace off` を実行してください．

### 実習5：RAMファイルとVFS

まず `/tmp/test.txt` にファイルを用意します．

```text
piconix# echo hello > /tmp/test.txt
piconix# trace run cat /tmp/test.txt
```

`open`，`read`，`write`，`close` を探し，`KERNEL` 行の `vfs_rw` などと見比べます．`(in_task_range)` のような1行表示は，同じ時刻に連続して記録された関数入口・出口を表示時にまとめたものです．記録自体は2件残っています．

### 実習6：Pythonからのファイル操作とADC

```text
piconix# trace run python test.py
```

`test.py` は内蔵温度センサを5回読み取るサンプルです．Pythonの代入や整数演算はユーザ空間の処理なので，それ自体は `SYSCALL` として現れません．一方，デバイスファイルの `open`，`read`，`write`，`close` や，待機に用いるシステムコールを観察できます．実際の処理は `/tmp/test.py` の内容も確認してください．

### 実習7：BH1750とI²Cのioctl

BH1750を接続した場合，1回だけ計測するコマンドを使います．

```text
piconix# trace run bh1750 1
```

`/dev/i2c/1` を開く `open`，転送を依頼する `ioctl`，終了時の `close` を確認します．関数計測ビルドでは，該当するSVC経路が呼ばれた場合に `i2c_driver_transfer` の記録も探します．センサ未接続では正常な測定を期待できません．`bh1750` を引数なしで実行すると連続測定になるため，`trace run` では回数を指定してください．

### 実習8：HTTPサーバでselect・accept・readを調べる

端末AでHTTPサーバを起動します．

```text
piconix# httpd
```

端末Bでは次を実行します．

```text
piconix# trace on
piconix# trace follow
```

PCのブラウザからPico WのHTTPサーバに接続し，`select`，`accept`，`read`，`write` などを探します．`select` の戻り値が準備できたFDの数を表すことと，後続の `accept`／`read` の関係に注目してください．`httpd` は通常継続動作するため，`trace run httpd` はここでは使いません．

### 実習9：ブロッキングacceptとI/O多重化の比較

端末Aで次を実行します．

```text
piconix# tcp_echod 7000
```

別端末で `trace on`／`trace follow` を実行し，外部TCPクライアントからポート7000に接続してデータを送ります．待機中と接続・データ到着後のイベントを観察します．さらに `nc` を使った通信と比較すると，単一ソケットのブロッキング待機と，複数FDを監視する `select` の使い分けを学べます．記録だけから各待機状態を断定せず，対応するユーザアプリとカーネルのソースを併せて確認してください．

### 実習10：PIDフィルタとSysTick

```text
piconix# ps
piconix# trace on
piconix# trace follow
```

一定間隔で現れる `TIMER TICKx1K` と `SWITCH` を探します．`TICKx1K` は1 msごとのSysTickを1,000回数えたときのイベントであり，すべての割込みを表示しているわけではありません．

特定のプロセスに絞る場合は，別途 `ps` で確認したPIDを用いて `trace follow PID` や `trace show PID` を実行します．PID指定は**表示フィルタ**であり，リングバッファの記録件数は減りません．`SWITCH` は切替先PIDで絞り込みます．割込み・非同期処理に由来するPIDなしの関数イベントを見たい場合は，PIDを付けずに `trace on -a` と `trace follow` を組み合わせます．

### 実習時の注意

- `PICONIX_KERNEL_FUNCTION_TRACE=OFF` のビルドでは，`KERNEL` 行は表示されません．
- `-a` はすべての関数を記録する指定ではなく，**コンパイル時に計測し，かつ選抜した関数**についてSVC外も許可する指定です．
- 記録領域は512イベントなので，負荷の高いプログラムでは古いイベントが上書きされます．短時間の観察から始めてください．
- 関数イベントの `Ret` は関数の戻り値ではありません．システムコールの戻り値と混同しないでください．
- トレースによる実行時間の増加があるため，表示時刻だけで厳密な性能測定はできません．

---

[演習一覧](README.md) ｜ [トレース操作ガイド](../guides/TRACE.md) ｜ [English](../../en/exercises/TRACE.md)
