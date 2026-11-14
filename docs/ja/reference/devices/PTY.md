# 擬似端末（PTY）

PicoNIXは，MCPだけに依存しない**汎用PTYを最大2組**提供します．物理TTYと同じように，子プロセスの標準入出力を端末として扱うための仮想デバイスです．MCPサーバからも利用でき，80列×24行の仮想画面とキー名入力を提供します．

## デバイスと制約

| ノード | 開き方 | 内容 |
|---|---|---|
| `/dev/ptmx` | `open(..., O_RDWR)` | Master．空き番号0・1を順に確保し，3組目は`OS_EBUSY` |
| `/dev/pts/0` | `open(..., O_RDWR)`など | Slave．Masterのopen後に利用可能 |
| `/dev/pts/0.screen` | `O_RDONLY` | 80×24の文字とANSI反転・カーソル属性（可変長ストリーム） |
| `/dev/pts/0.key` | `O_WRONLY` | `key:`・`text:`による特殊キー・通常文字の入力 |
| `/dev/pts/1`，`/dev/pts/1.screen`，`/dev/pts/1.key` | 同上 | 2組目のSlave・画面・キー入力 |

- Masterの`write()`がSlaveへの入力になり，TTYのcanonical入力・echo・Ctrl+Cなどの行規律を通ります．Masterの`read()`はSlaveからの出力を受け取ります．
- SlaveはそれぞれTTY番号3（PTY 0）と5（PTY 1）で，`ioctl(fd, OS_TTY_*, ...)`と`fcntl()`に対応します．`tty`コマンドは対応する`/dev/pts/N`を表示します．
- `spawn()`は標準FD 0〜2を継承します．親がFD 0をSlaveに変更して起動すると，**子だけ**制御TTYがPTYになります．前景起動には`spawn_foreground()`を利用します．親のFDと制御TTYは自動では変更されません．
- MasterとSlaveの間にはソケットや管理専用プロセスを使いません．出力リングは1組あたり1024バイトです．カーネルは最小限のANSI解析を行って現在画面を保持します．
- Slaveの出力がリング満杯ならブロッキング`write()`は空きを待ち，`O_NONBLOCK`なら`OS_EAGAIN`を返します．部分書込みもあります．Masterへの入力書込みはバッファ満杯時に`OS_EAGAIN`となり，呼出側が後で再試行します．
- Slaveをすべて閉じて出力を読み切ったMasterの`read()`は0（EOF）です．Masterがすべて閉じられたSlaveの`read()`は，残りの入力を消費した後に0を返し，`write()`は`OS_EIO`です．両端を閉じた後は初期状態へ戻ります．
- 各組は，そのMasterとSlaveをすべて閉じた後に再利用できます．FDの複製は`dup()`と`dup2()`が利用可能です．

### 汎用PTYの画面専用モード

`mcpd`は起動時にMasterと`sh`を起動します．Masterを開いた後，Slaveを開く前に`ioctl(master, OS_PTY_SCREEN_ONLY, 1)`を設定します．このモードではSlaveの出力を1024バイトの出力リングに蓄積せず，割り当てられた`/dev/pts/N.screen`の仮想画面だけを直接更新します．出力リングのdrainや固定200 ms待機は不要です．Masterの生出力`read()`は使えません．通常のPTY（`pty_test`など）は変更されず，出力リングとMasterの`read()`を利用できます．両端が閉じられた時点でモードも解除されます．

`terminal_send`は入力を受け付けたバイト数のみ返し，描画完了を待ちません．画面確認はLLMが必要に応じて`terminal_read`を呼び出します．画面の読取りは原子的なスナップショットではありません．


### スレーブ番号の取得

`/dev/ptmx`の各openは空いている最小番号を割り当てます．`/dev/pts/N`を固定の番号で開かず，以下のように番号を取得してください．

```c
int master = open("/dev/ptmx", O_RDWR | O_NONBLOCK);
int n = ioctl(master, OS_PTY_GET_INDEX, 0); /* 0または1 */
char path[32];
snprintf(path, sizeof path, "/dev/pts/%d", n);
int slave = open(path, O_RDWR);
```

`OS_PTY_GET_INDEX`（0x20c）はマスターFDでのみ有効です．既存の`mcpd`と`pty_test`もこの方式を使用します．

## 利用の流れ

1. Masterを`O_RDWR`で開き，`ioctl(master, OS_PTY_GET_INDEX, 0)`で0または1を取得し，対応する`/dev/pts/N`を開きます．
2. 親のFD 0〜2を`dup()`で退避します．Slaveを`dup2(slave, 0/1/2)`で接続し，`spawn_foreground()`で子を起動します．
3. 親の標準FDを復元し，親側のSlave FDを閉じます．
4. Masterにキー入力を`write()`し，MasterからANSI出力を`read()`します．必要なら`select()`で入出力を多重化します．
5. 子プロセスの終了を`waitpid()`で確認し，Masterを閉じます．

`vi`などの全画面アプリはANSIシーケンスを出力します．PTYの生出力はMasterから読み取れます．PicoNIX独自の`.screen`は必要最小限のCSI制御を解釈した文字画面を返します．セル単位の反転表示を `ESC[7m` / `ESC[0m` で出力します．また，端末カーソルの1文字を `ESC[4m` / `ESC[24m`（下線開始・解除）で囲みます．カーソル文字が空白でも同様です．`ESC[?25l`でカーソルを隠した間はSGR 4/24を出力せず，`ESC[?25h`で再び表示します．読み出しサイズは可変で，ANSIシーケンスの途中で分割される場合があります．下線は画面読取り時だけ合成され，文字属性として保存しません．


## 実機テスト（標準コマンド）

`pty_test`（アプリID 56）は通常ビルドに含まれます．ソースは`userland/apps/pty_test.c`です．ビルドオプションの追加指定は不要です．

```text
piconix# pty_test
/dev/pts/0  (空き番号によっては /dev/pts/1)
```

引数なしでは`tty`をPTY上で起動し，制御端末のパスを確認できます．引数を指定すると，そのコマンドの標準入出力をPTYに接続し，現在の端末と双方向中継します．

```text
piconix# pty_test fd
piconix# pty_test vi /tmp/test.txt
piconix# pty_test tty
/dev/pts/0  (空き番号によっては /dev/pts/1)
```

中継中は元の端末を非カノニカル入力・エコーOFFにし，`select()`で実コンソールとPTY Masterを監視します．終了後は端末設定とFDの状態を復元します．子プロセスの終了に伴うPTYのEOFまで出力を取得します．

**Ctrl+C：** `pty_test`は元のTTYの割込み解釈を一時的に無効化し，`0x03`をMasterへ転送します．PTY SlaveではCtrl+Cを前景タスクの強制終了として処理します．`pty_test sh`から`fd`を起動した場合は，`fd`が前景なら`fd`を終了させ，シェルへ復帰することを想定しています．本格的なPOSIXシグナルではなく，アプリの終了処理は呼ばれません．TTY設定は既存の前景タスク終了処理で復元される場合がありますが，画面が乱れたら`reset`や`stty`で復旧してください．画面サイズは80×24に固定し，カーソル位置照会（CSI 6n）に応答します．`pty_test`はMCPツールではありません．

`OS_TTY_GET_INTR`（0x209）と`OS_TTY_SET_INTR`（0x20a）は，そのTTYでCtrl+Cを中断として扱うかを取得・設定するPicoNIX独自の`ioctl()`です．デフォルトは1（有効）です．0にすると，非カノニカル入力では`0x03`を通常文字として読み取れます．`pty_test`は元のTTYで一時的に0を設定し，終了時に復元します．

**実装：** `kernel/kernel.c`の`pty_ref()`，`pty_master_rw()`，`pty_slave_write()`，`wake_ptys()`，`spawn_in_slot()`，`foreground_child()`．デバイス登録は`include/device_names.h`．テストは`tools/test_scripts/test_pty_runtime.py`です．

[デバイス一覧](README.md) ｜ [TTY](../commands/TTY.md) ｜ [English](../../../en/reference/devices/PTY.md)

## 仮想画面とキー入力

**内部実装：** [スクリーン管理モジュール](../../development/kernel/SCREEN-MANAGEMENT.md)．



```sh
cat /dev/pts/0.screen
echo 'key:DOWN' > /dev/pts/0.key
echo 'key:CTRL_C' > /dev/pts/0.key
echo 'text:ls\n' > /dev/pts/0.key
```

`/dev/pts/0`と`/dev/pts/1`および各`.screen`・`.key`は，それぞれのMasterが開かれている間だけ`ls /dev/pts`に現れます．Masterを閉じてもSlaveの参照が残る場合は，最後のSlaveを閉じるまでその番号を再利用しません．画面は80列×24行に固定され，履歴やスクロールバックはありません．`.screen`の各行は空白を含む80文字と改行で構成され，`read()`のFD位置に応じて分割取得できます．読み出し中に画面が更新された場合の原子的なスナップショットは保証しません．

`.key`は**1回の`write()`につき1つの入力命令**を受け取ります．接頭辞なしの旧式キー名は受け付けません．

- `key:X`：1文字キー，または`UP`・`DOWN`・`LEFT`・`RIGHT`・`ENTER`・`ESC`・`ESCAPE`・`TAB`・`SPACE`・`SP`・`BACKSPACE`・`BS`・`DELETE`・`CTRL_A`〜`CTRL_Z`を指定します．
- `text:...`：通常文字を入力します．`\n`はLF，`\\`はバックスラッシュに変換します．未知のエスケープはエラーとなります．
- `echo`が付加する末尾のLF（および直前のCR）は命令の終端として除去します．改行を実際に入力したい場合は`text:ls\n`または`key:ENTER`を指定します．
- 出力は最大127バイトで，長い入力は分割した**独立の**`text:`命令で送信してください．コマンドを複数回の`write()`に分割しても結合されません．
- 不正な接頭辞・キー・エスケープはPTYへ何も入力せず`OS_EINVAL`となります．PTY入力キューの混雑による部分的な入力受付は従来のPTY送信規則に従います．


カーネルのANSI解析は完全な端末エミュレータではありません．基本的なカーソル移動・行消去・画面消去・SGR反転・代替画面切替・CSI 6nの応答を対象としています．日本語などの複数バイト文字・広範な端末制御・ウィンドウサイズの変更は対象外です．通常のPTYバイト中継は維持しています．

MCPでの利用方法は[こちら](../commands/MCP.md)を参照してください．

## カーソル位置の取得

カーソル位置は`/dev/pts/N.screen`内のANSI SGR 4/24で囲まれたセルとして表現されます．MCPの`terminal_read`もこの画面情報を転送し，別途行・列を表示しません．カーソルが表示されていれば，移動だけでも画面データが変わるため変更として検出できます．
