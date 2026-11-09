# 擬似端末（PTY）

PicoNIXは，MCPだけに依存しない**汎用PTYを1組**提供します．物理TTYと同じように，子プロセスの標準入出力を端末として扱うための仮想デバイスです．この段階ではMCPからPTYを制御するツールは未実装です．

## デバイスと制約

| ノード | 開き方 | 内容 |
|---|---|---|
| `/dev/ptmx` | `open(..., O_RDWR)` | Master．1組だけ確保でき，重複openは`OS_EBUSY` |
| `/dev/pts/0` | `open(..., O_RDWR)`など | Slave．Masterのopen後に利用可能 |

- Masterの`write()`がSlaveへの入力になり，TTYのcanonical入力・echo・Ctrl+Cなどの行規律を通ります．Masterの`read()`はSlaveからの出力を受け取ります．
- SlaveはTTY番号3で，`ioctl(fd, OS_TTY_*, ...)`と`fcntl()`に対応します．`tty`コマンドは`/dev/pts/0`を表示します．
- `spawn()`は標準FD 0〜2を継承します．親がFD 0をSlaveに変更して起動すると，**子だけ**制御TTYがPTYになります．前景起動には`spawn_foreground()`を利用します．親のFDと制御TTYは自動では変更されません．
- MasterとSlaveの間にはソケットや管理専用プロセスを使いません．出力リングは1024バイトです．カーネルはANSI画面解析を行いません．
- Slaveの出力がリング満杯ならブロッキング`write()`は空きを待ち，`O_NONBLOCK`なら`OS_EAGAIN`を返します．部分書込みもあります．Masterへの入力書込みはバッファ満杯時に`OS_EAGAIN`となり，呼出側が後で再試行します．
- Slaveをすべて閉じて出力を読み切ったMasterの`read()`は0（EOF）です．Masterがすべて閉じられたSlaveの`read()`は，残りの入力を消費した後に0を返し，`write()`は`OS_EIO`です．両端を閉じた後は初期状態へ戻ります．
- 新規セッションは，前セッションのMasterとSlaveをすべて閉じるまで作れません．FDの複製は`dup()`と`dup2()`が利用可能です．

## 利用の流れ

1. Masterを`O_RDWR`で開き，続いてSlaveを開きます．
2. 親のFD 0〜2を`dup()`で退避します．Slaveを`dup2(slave, 0/1/2)`で接続し，`spawn_foreground()`で子を起動します．
3. 親の標準FDを復元し，親側のSlave FDを閉じます．
4. Masterにキー入力を`write()`し，MasterからANSI出力を`read()`します．必要なら`select()`で入出力を多重化します．
5. 子プロセスの終了を`waitpid()`で確認し，Masterを閉じます．

`vi`などの全画面アプリはANSIシーケンスを出力します．実画面の構築はMaster側の端末エミュレータが行います．これはカーネルTTYとは別の責務です．既存のテストはホスト側での入出力検証であり，実機での`vi`実行確認は今後必要です．


## 実機テスト（標準コマンド）

`pty_test`（アプリID 56）は通常ビルドに含まれます．ソースは`userland/apps/pty_test.c`です．ビルドオプションの追加指定は不要です．

```text
piconix# pty_test
/dev/pts/0
```

引数なしでは`tty`をPTY上で起動し，制御端末のパスを確認できます．引数を指定すると，そのコマンドの標準入出力をPTYに接続し，現在の端末と双方向中継します．

```text
piconix# pty_test fd
piconix# pty_test vi /tmp/test.txt
piconix# pty_test tty
/dev/pts/0
```

中継中は元の端末を非カノニカル入力・エコーOFFにし，`select()`で実コンソールとPTY Masterを監視します．終了後は端末設定とFDの状態を復元します．子プロセスの終了に伴うPTYのEOFまで出力を取得します．

**Ctrl+C：** `pty_test`は元のTTYの割込み解釈を一時的に無効化し，`0x03`をMasterへ転送します．PTY SlaveではCtrl+Cを前景タスクの強制終了として処理します．`pty_test sh`から`fd`を起動した場合は，`fd`が前景なら`fd`を終了させ，シェルへ復帰することを想定しています．本格的なPOSIXシグナルではなく，アプリの終了処理は呼ばれません．TTY設定は既存の前景タスク終了処理で復元される場合がありますが，画面が乱れたら`reset`や`stty`で復旧してください．画面サイズの変更通知やANSI画面の解釈は提供しません．`pty_test`はMCPツールではありません．

`OS_TTY_GET_INTR`（0x209）と`OS_TTY_SET_INTR`（0x20a）は，そのTTYでCtrl+Cを中断として扱うかを取得・設定するPicoNIX独自の`ioctl()`です．デフォルトは1（有効）です．0にすると，非カノニカル入力では`0x03`を通常文字として読み取れます．`pty_test`は元のTTYで一時的に0を設定し，終了時に復元します．

**実装：** `kernel/kernel.c`の`pty_ref()`，`pty_master_rw()`，`pty_slave_write()`，`wake_ptys()`，`spawn_in_slot()`，`foreground_child()`．デバイス登録は`include/device_names.h`．テストは`tools/test_scripts/test_pty_runtime.py`です．

[デバイス一覧](README.md) ｜ [TTY](../commands/TTY.md) ｜ [English](../../../en/reference/devices/PTY.md)
