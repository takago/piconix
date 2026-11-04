# PicoNIX カーネルリーディングガイド

*開発ガイド*

このガイドは，PicoNIXの**現行ソースを読んでOSの動作原理を理解する**ための道順です．機能の追加手順は[カーネル拡張ガイド](KERNEL-DEVELOPMENT.md)を参照してください．ソースをファイルの先頭から順番に読む必要はありません．**起動 → タスクの準備 → スケジューリング → 例外 → システムコール → ファイル → TTY・パイプ → デバイス → ネットワーク**の順で，関心のある処理を追います．

## 読み進め方

1. 各段階の「読む順序」に従ってソースを開き，該当関数のコメント・引数・戻り値を確認します．
2. 呼び出し元と呼び出し先を探し，処理の流れを紙などに図示します．`rg -n '関数名\(' kernel userland`が使えます．
3. 可能なら実機でコマンドを実行し，`ps`・`trace`・`mem`などで結果を確認します．機器に依存する実験は省略可能です．
4. 各段階の確認問題に答えてから次へ進みます．まず**既存のコードを変更せず**読むことを推奨します．

事前知識はCの関数・構造体・ポインタと基礎的な二進数です．第4段階だけはCortex-M0+の例外処理とThumbアセンブリを補助資料で確認します．最初に[全体構造](../../reference/ARCHITECTURE.md)，[メモリ配置](../../reference/MEMORY-LAYOUT.md)，[Linuxとの違い](../../reference/DIFFERENCES.md)に目を通すと理解しやすくなります．**PicoNIXは一般的なLinuxのプロセス分離・仮想記憶をそのまま実装したOSではありません．**

| 段階 | 学習テーマ | 主なファイル | 学ぶこと |
|---|---|---|---|
| 1 | 起動 | `kernel/main.c` | 初期化，プログラム登録，最初のシェル |
| 2 | タスクとメモリ | `kernel/kernel.c`，`include/piconix.h` | 固定長スロット，プログラムイメージ，初期スタック |
| 3 | スケジューリング | `kernel/kernel.c` | SysTick，READY，BLOCKED，アイドル |
| 4 | コンテキスト切替 | `kernel/context.S` | PendSV，PSP，レジスタ退避・復元 |
| 5 | システムコール | `userland/lib/syscalls.c`，`kernel/context.S`，`kernel/kernel.c` | SVCとカーネルへの入口 |
| 6 | ファイル・FD | `kernel/kernel.c` | open/read/write/close，VFS，/tmp，/proc |
| 7 | TTY・パイプ | `kernel/kernel.c`，`userland/apps/shell.c` | 入力待ち，FDの複製，EOF，起床 |
| 8 | デバイス | `kernel/gpio.c`，`kernel/fb.c`など | VFSとデバイスドライバの役割分担 |
| 9（任意） | ネットワーク（Pico W） | `kernel/net.c`，`kernel/net_socket.c`など | Wi-Fi，lwIP，ソケットの役割分担 |

> **ファイルの探し方：** 関数定義と概説は[関数一覧](../../../FUNCTIONS.md)を利用してください．同一覧の行番号はソースを編集すると変わるため，本ガイドでは固定の行番号を記しません．

## 第1段階：カーネルの起動

**読む順序：** `kernel/main.c`の`main()` → `os_init()` → `register_image()` → `net_init()` → `os_init_idle()` → `os_start_program(2)` → `os_start()`．`idle_task()`も読みます．

`register_image()`はFlash上の実行イメージを**登録**し，その時点ではユーザスロットへコピーしません．シェルのプログラムIDは2です．`idle_task()`では`net_poll()`と`os_console_poll()`が動きます．

**確認問題：** 登録と起動は何が違うか．なぜアイドルタスクが必要か．

## 第2段階：タスク生成とメモリ

**読む順序：** `include/piconix.h`の`OS_MAX_TASKS`・`OS_TASK_BYTES` → `kernel/kernel.c`の`os_register_program()` → `os_register_ram_image()` → `free_program_slot()` → `spawn_in_slot()` → `os_start_program()`．`spawn_in_slot()`には前方宣言と定義があるので，定義の本体を読みます．

プログラムの登録情報と実行中タスクの状態を区別し，Flashのイメージ，RAM上の固定長タスクスロット，初期スタックのつながりを追います．スタック・レジスタ初期化が第4段階の復帰処理にどのようにつながるかも控えます．

**実験：** `ps`でタスクを確認し，`mem -s kernel`や`mem -s sh`でシンボルを調べます．詳しい操作は[メモリ参照](../../guides/MEM.md)を参照してください．

**確認問題：** プログラムID・PID・スロット番号は同じか．RAMスロットはいつ確保されるか．

## 第3段階：スケジューラ

**読む順序：** `os_start()` → `isr_systick()` → `pend()` → `os_schedule()`．補助としてタスク状態の定義と`wake_readers()`・`wake_waiters()`・`wake_pipes()`を探します．

SysTickは1 ms周期で時刻を進め，通常10 tickごとにPendSVを要求します．割込みのたびに重いタスク選択処理を直接行うのではなく，PendSV側から`os_schedule()`を呼びます．READYでないタスクとアイドルタスクの扱いも確認してください．Pico Wではネットワークのためアイドル側へ実行機会を渡す分岐があります．

**実験：** 複数のコマンドが動く状態で`ps`を確認します．コンテキスト切替の観察方法は[トレース](../../guides/TRACE.md)を参照してください．

**確認問題：** SysTickとPendSVは何を分担しているか．実行可能なユーザタスクがない場合どうなるか．

## 第4段階：コンテキストスイッチ

**読む順序：** `kernel/context.S`の`isr_pendsv` → `kernel/kernel.c`の`os_schedule()` → `isr_pendsv`に戻って復元処理．

PSP（プロセススタック），MSP（例外処理側のスタック），ハードウェアによる例外フレームの保存と，ソフトウェアによるr4〜r11の退避・復元を区別します．`os_schedule(oldsp)`が旧PSPを保存し次のPSPを返す点が中心です．

**確認問題：** タスクを切り替えたあとに，別のタスクの続きから実行できるのはなぜか．SVCとPendSVは目的がどう違うか．

## 第5段階：システムコール

**読む順序：** `userland/apps/hello.c`（または別の`write()`使用アプリ） → `userland/lib/syscalls.c`の`write()`と`call()` → `include/piconix_syscall.h` → `kernel/context.S`の`isr_svcall` → `kernel/kernel.c`の`os_svc_dispatch()`．

```text
ユーザプログラム
  → write(fd, buffer, size)
  → call(SYS_WRITE, ...)
  → svc #0
  → isr_svcall
  → os_svc_dispatch(例外フレーム)
  → vfs_rw() などのカーネル処理
  → 戻り値をユーザ側へ
```

`call()`はr0にシステムコール番号，r1〜r3に引数を置きます．`os_svc_dispatch()`の`SYS_WRITE`分岐を読み，バッファの範囲検査を行う場所も探してください．SVCは「別のアドレス空間へ移動する」ことと同義ではありません．PicoNIXのメモリ保護上の限界は[内部構造](../../reference/INTERNAL.md)を参照してください．

**実験：** `trace run hello`でwriteのENTER/EXITを観察します．

**確認問題：** ユーザコードが直接デバイスを操作する代わりにSVCを使う理由は何か．

## 第6段階：ファイルとFD

**読む順序：** `fd_set()` → `fd_release()` → `vfs_open()` → `vfs_rw()` → `tmp_open()`・`tmp_rw()` → `device_rw_proc()`・`device_rw_tty()`．最後に`os_svc_dispatch()`の`SYS_OPEN`・`SYS_READ`・`SYS_WRITE`・`SYS_CLOSE`を見直します．

FD番号と，FDに結び付けられた対象の内部ID・ファイル位置・フラグは別の情報です．`/tmp`のRAMファイル，`/proc`の仮想情報，`/dev`のデバイスは，パスの振り分け先が異なります．

**実験：** `cat /proc/uptime`，`ls /dev`，`lsof`を使い，[ファイルシステム](../../reference/FILESYSTEM.md)と比較します．

**確認問題：** 同じ`read()`が通常ファイルとデバイスの双方で動くのはなぜか．EOFはどのように表すか．

## 第7段階：TTYとパイプ

**TTYを読む順序：** `tty_input_char()` → `tty_enqueue()` → `tty_dequeue()` → `device_rw_tty()` → `wake_readers()`．[端末](../../guides/TERMINAL.md)と[TTY設定](../../guides/TTY.md)も参照します．

**パイプを読む順序：** `pipe_ref()` → `pipe_transfer()` → `wake_pipes()` → `fd_set()`・`fd_release()` → `os_svc_dispatch()`の`SYS_PIPE`・`SYS_DUP2`・`SYS_CLOSE` → シェルの`|`処理．

```sh
echo hello | grep hello | cat
```

**確認問題：** パイプの読取側がデータを待つときタスクはどうなるか．書込側の参照がなくなったことをどこで判断し，EOFを返すか．TTYの入力行編集と画面制御は誰の責任か．

## 第8段階：デバイス

**読む順序：** `kernel/kernel.c`のデバイスI/O振り分け → `kernel/gpio.c` → `kernel/fb.c` → `kernel/adc.c` → `kernel/i2c.c` → `kernel/pwm.c`．ハードウェアの定義は`kernel/pins.h`・`kernel/devices.h`・`include/device_names.h`も参照します．

GPIOなどのデバイス固有処理が，VFSと`read()`・`write()`・`ioctl()`の入口からどのように呼ばれるかを調べます．**ネットワークはこの段階では扱いません．** [教科書 B5](../../learn/B/B5.md)と[ドライバ開発](../drivers/DRIVER-DEVELOPMENT.md)も参照してください．

**確認問題：** デバイス固有のコードを通常のI/O入口から切り離す利点は何か．VFS側とドライバ側では何を担当するか．

## 第9段階：ネットワーク（Pico Wのみ・任意）

**読む順序：** `kernel/main.c`での`net_init()` → `kernel/net.c`の`net_init()`・`net_poll()` → `kernel/net_socket.c` → `kernel/net_service_telnet.c`・`kernel/net_service_remote_devctl.c` → `userland/apps/httpd.c`．

Wi-Fi接続・lwIPのポーリング，カーネル内のTelnet／UDPデバイス制御，ユーザアプリのTCP/UDPソケットを区別します．ソケットAPIとユーザ側の例は[ネットワークプログラミング](../apps/c/NETWORK-PROGRAMMING.md)を参照してください．**通常のPicoだけで学ぶ場合はこの段階を省略できます．**

**補助資料：** [ネットワーク実装・拡張ガイド](../network/NETWORK-DEVELOPMENT.md)で各ファイルの責任分担と改造時の注意点を確認できます．

**確認問題：** `net_poll()`はなぜ必要か．`httpd`がユーザプログラムなのに，Telnetサービスはカーネル側にあるのはどのような設計上の違いか．

## 最終演習：一つの操作を端から端まで追う

`trace run hello`を実行したうえで，**「シェルがhelloを起動し，文字列がTTYへ出力され，終了するまで」**を次の順に説明してください．

1. シェルの起動要求（`spawn()`）とSVC
2. 実行イメージの読み込み・タスクの準備
3. スケジューラとPendSVによる最初の実行
4. `write()`からVFSとTTYへの経路
5. 終了処理，FDの解放，スロットの再利用

余力があれば，`echo hello | cat`の起動・FD複製・ブロック・EOFまで同じ方法で図示します．**コードの変更が目的ではなく，関数間の関係を説明できることが到達目標です．**

## 次に読む文書

- [関数一覧](../../../FUNCTIONS.md)：定義箇所と処理概要の索引
- [内部構造](../../reference/INTERNAL.md)：メモリ・スケジューラ・SVCの補足
- [システムコール一覧](../../reference/SYSCALLS.md)：番号，引数と動作
- [システムコール開発](../syscalls/SYSCALL-DEVELOPMENT.md)：自分で呼び出しを追加する
- [カーネル拡張ガイド](KERNEL-DEVELOPMENT.md)：機能を変更する実習

---

**関連リンク：** [開発ガイド一覧](../README.md) ｜ [Part C 教科書](../../learn/C/README.md) ｜ [文書案内](../../../DOCS.ja.md) ｜ [English](../../../en/development/kernel/KERNEL-READING-GUIDE.md)
