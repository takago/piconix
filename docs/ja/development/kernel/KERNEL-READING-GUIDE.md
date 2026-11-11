# PicoNIX カーネルソース・処理経路索引

*開発ガイド*

この資料は，カーネルの機能とソースファイル，主要な呼び出し経路を対応付ける索引です．変更手順は[カーネル拡張ガイド](KERNEL-DEVELOPMENT.md)を参照してください．

## ソースファイルの対応関係

例えば`write()`のカーネル内経路は，`call()`，`isr_svcall`，`os_svc_dispatch()`，`vfs_rw()`の順に通ります．実際の呼び出し元・呼び出し先は`rg -n '関数名\(' kernel userland`で検索できます．`trace run hello`は実行時のイベントを表示しますが，すべての関数呼び出しを記録するものではありません．

UNIX/Linuxのプロセス・FD・システムコール，C言語，Cortex-M0+の例外処理の知識を前提にします．関連する仕様は[全体構造](../../reference/ARCHITECTURE.md)，[メモリ配置](../../reference/MEMORY-LAYOUT.md)，[Linuxとの違い](../../reference/DIFFERENCES.md)にあります．**PicoNIXは一般的なLinuxのプロセス分離・仮想記憶をそのまま実装したOSではありません．**

| 分野 | 対象機能 | 主なファイル | 内容 |
|---|---|---|---|
| 起動 | 起動 | `kernel/main.c` | 初期化，プログラム登録，最初のシェル |
| タスク | タスクとメモリ | `kernel/kernel.c`，`include/piconix.h` | 固定長スロット，プログラムイメージ，初期スタック |
| スケジューラ | スケジューリング | `kernel/kernel.c` | SysTick，READY，BLOCKED，アイドル |
| 例外 | コンテキスト切替 | `kernel/context.S` | PendSV，PSP，レジスタ退避・復元 |
| SVC | システムコール | `userland/lib/syscalls.c`，`kernel/context.S`，`kernel/kernel.c` | SVCとカーネルへの入口 |
| VFS | ファイル・FD | `kernel/kernel.c` | open/read/write/close，VFS，/tmp，/proc |
| 端末・IPC | TTY・パイプ | `kernel/kernel.c`，`userland/apps/shell.c` | 入力待ち，FDの複製，EOF，起床 |
| デバイス | デバイス | `kernel/gpio.c`，`kernel/fb.c`など | VFSとデバイスドライバの役割分担 |
| ネットワーク | ネットワーク（Pico W） | `kernel/net.c`，`kernel/net_socket.c`など | Wi-Fi，lwIP，ソケットの役割分担 |

> **ファイルの探し方：** 関数定義と概説は[関数一覧](../FUNCTIONS.md)を利用してください．同一覧の行番号はソースを編集すると変わるため，本ガイドでは固定の行番号を記しません．

## カーネルの起動

**処理経路：** `kernel/main.c`の`main()` → `os_init()` → `register_image()` → `net_init()` → `os_init_idle()` → `os_start_program(2)` → `os_start()`．`idle_task()`も読みます．

`register_image()`はFlash上の実行イメージを**登録**し，その時点ではユーザスロットへコピーしません．シェルのプログラムIDは2です．`idle_task()`では`net_poll()`と`os_console_poll()`が動きます．


## タスク生成とメモリ

**処理経路：** `include/piconix.h`の`OS_MAX_TASKS`・`OS_TASK_BYTES` → `kernel/kernel.c`の`os_register_program()` → `os_register_ram_image()` → `free_program_slot()` → `spawn_in_slot()` → `os_start_program()`．`spawn_in_slot()`には前方宣言と定義があるので，定義の本体を読みます．

プログラムの登録情報と実行中タスクの状態を区別し，Flashのイメージ，RAM上の固定長タスクスロット，初期スタックのつながりを追います．スタック・レジスタ初期化がコンテキスト切替の復帰処理にどのようにつながるかも控えます．

**観測例：** `ps`でタスクを確認し，`mem -s kernel`や`mem -s sh`でシンボルを調べます．詳しい操作は[メモリ参照](../../reference/commands/MEM.md)を参照してください．


## スケジューラ

**処理経路：** `os_start()` → `isr_systick()` → `pend()` → `os_schedule()`．補助としてタスク状態の定義と`wake_readers()`・`wake_waiters()`・`wake_pipes()`を探します．

SysTickは1 ms周期で時刻を進め，通常10 tickごとにPendSVを要求します．割込みのたびに重いタスク選択処理を直接行うのではなく，PendSV側から`os_schedule()`を呼びます．READYでないタスクとアイドルタスクの扱いも確認してください．Pico Wではネットワークのためアイドル側へ実行機会を渡す分岐があります．

**観測例：** 複数のコマンドが動く状態で`ps`を確認します．コンテキスト切替の観察方法は[トレース](../../reference/commands/TRACE.md)を参照してください．


## コンテキストスイッチ

**処理経路：** `kernel/context.S`の`isr_pendsv` → `kernel/kernel.c`の`os_schedule()` → `isr_pendsv`に戻って復元処理．

PSP（プロセススタック），MSP（例外処理側のスタック），ハードウェアによる例外フレームの保存と，ソフトウェアによるr4〜r11の退避・復元を区別します．`os_schedule(oldsp)`が旧PSPを保存し次のPSPを返す点が中心です．


## システムコール

**処理経路：** `userland/apps/hello.c`（または別の`write()`使用アプリ） → `userland/lib/syscalls.c`の`write()`と`call()` → `include/piconix_syscall.h` → `kernel/context.S`の`isr_svcall` → `kernel/kernel.c`の`os_svc_dispatch()`．

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

**観測例：** `trace run hello`でwriteのENTER/EXITを観察します．


## ファイルとFD

**処理経路：** `fd_set()` → `fd_release()` → `vfs_open()` → `vfs_rw()` → `tmp_open()`・`tmp_rw()` → `device_rw_proc()`・`device_rw_tty()`．最後に`os_svc_dispatch()`の`SYS_OPEN`・`SYS_READ`・`SYS_WRITE`・`SYS_CLOSE`を見直します．

FD番号と，FDに結び付けられた対象の内部ID・ファイル位置・フラグは別の情報です．`/tmp`のRAMファイル，`/proc`の仮想情報，`/dev`のデバイスは，パスの振り分け先が異なります．

**観測例：** `cat /proc/uptime`，`ls /dev`，`lsof`を使い，[ファイルシステム](../../reference/FILESYSTEM.md)と比較します．


## TTYとパイプ

**TTYの処理経路：** `tty_input_char()` → `tty_enqueue()` → `tty_dequeue()` → `device_rw_tty()` → `wake_readers()`．[端末](../../reference/commands/TERMINAL.md)と[TTY設定](../../reference/commands/TTY.md)も参照します．

**パイプの処理経路：** `pipe_ref()` → `pipe_transfer()` → `wake_pipes()` → `fd_set()`・`fd_release()` → `os_svc_dispatch()`の`SYS_PIPE`・`SYS_DUP2`・`SYS_CLOSE` → シェルの`|`処理．

```sh
echo hello | grep hello | cat
```


## デバイス

**処理経路：** `kernel/kernel.c`のデバイスI/O振り分け → `kernel/gpio.c` → `kernel/fb.c` → `kernel/adc.c` → `kernel/i2c.c` → `kernel/pwm.c`．ハードウェアの定義は`kernel/pins.h`・`kernel/devices.h`・`include/device_names.h`も参照します．

GPIOなどのデバイス固有処理が，VFSと`read()`・`write()`・`ioctl()`の入口からどのように呼ばれるかを調べます．ネットワーク側の処理経路は別項に示します． [デバイス仕様](../../reference/devices/README.md)と[ドライバ開発](../drivers/DRIVER-DEVELOPMENT.md)も参照してください．


## ネットワーク（Pico W）

**処理経路：** `kernel/main.c`での`net_init()` → `kernel/net.c`の`net_init()`・`net_poll()` → `kernel/net_socket.c` → `kernel/net_service_telnet.c`・`kernel/net_service_remote_devctl.c` → `userland/apps/httpd.c`．

Wi-Fi接続・lwIPのポーリング，カーネル内のTelnet／UDPデバイス制御，ユーザアプリのTCP/UDPソケットを区別します．ソケットAPIとユーザ側の例は[ネットワークプログラミング](../apps/c/NETWORK-PROGRAMMING.md)を参照してください．ネットワーク側の処理はPico W専用です．

**補助資料：** [ネットワーク実装・拡張ガイド](../network/NETWORK-DEVELOPMENT.md)で各ファイルの責任分担と改造時の注意点を確認できます．


## CPUモード・スタック・SVC・PendSVの実装

この節ではCortex-M0+のアーキテクチャ上の動作とPicoNIX固有の処理を区別します．以下の関数名・構造体メンバは`kernel/context.S`と`kernel/kernel.c`で確認できます．

### 1．Thread／Handlerモードと特権レベル

Cortex-M0+では，通常のコードは**Threadモード**，SVC・PendSV・SysTick・外部割り込みのハンドラは**Handlerモード**で動作します．モードと特権は別の概念です．Handlerモードは常に特権付きですが，Threadモードは特権付きでも非特権でも動作できます．PicoNIXのユーザタスクは特権付きThreadモードでPSPを使用します．Linuxのようなユーザ／カーネル間のハードウェア保護を実現しているわけではありません．

| 状況 | CPUモード | 使用するスタック | 関連ソース |
|---|---|---|---|
| PicoNIXのユーザタスク | Thread | タスクごとのPSP | `kernel/kernel.c`の`spawn_in_slot()` |
| システムコール | Handler（SVC） | 共通MSP | `kernel/context.S`の`isr_svcall` |
| スケジューリング | Handler（PendSV） | 共通MSP | `kernel/context.S`の`isr_pendsv` |
| 時刻更新・割り込み | Handler | 共通MSP | `kernel/kernel.c`の`isr_systick()` |

Handlerモードは「割り込み禁止モード」ではありません．現在の例外よりも**高優先度**の例外は，マスクされていなければ割り込めます．また，Handlerモードの役割はスタック切替だけではなく，例外の優先度と復帰先をCPUが管理することにもあります．

### 2．PSP・MSPと例外フレーム

**PSPとMSPはメモリ領域の名前ではなく，CPUのスタックポインタレジスタ**です．PicoNIXは各タスクの保存済みPSPを`task_t.sp`で管理し，例外処理用のMSPを共通利用します．タスクの作成時には`spawn_in_slot()`が初期スタックフレームを組み立てます．アイドルタスクの初期化は`os_init_idle()`です．

ThreadモードでPSPを選択しているときに例外が起きると，CPUはまず**現在のPSP側**に`r0〜r3，r12，lr，pc，xPSR`を自動保存します．その後，Handlerモードの命令実行にはMSPを使います．したがって，ユーザのレジスタが最初からMSPへ積まれるわけではありません．

```text
タスクAのPSP側スタック           共通MSP側スタック
  r4〜r11（PendSVが保存）        SVC/PendSVなどのハンドラが使用
  r0〜r3, r12, lr, pc, xPSR       （カーネル関数の一時的な呼出し状態）
  ↑ 例外発生時の実行状態
```

`kernel/context.S`の`isr_pendsv`では，旧タスクの`r4〜r11`をPSP側に退避し，`os_schedule(oldsp)`へその位置を渡します．選ばれたタスクの保存済みフレームから`r4〜r11`とPSPを復元し，`0xFFFFFFFD`による例外復帰でPSPを使うThreadモードに戻ります．**CPU内のPSPレジスタは1本ですが，保存済みPSPはタスクごと**です．

### 3．SVCとPendSVは別の例外

**SVCはシステムコールの入口**です．`userland/lib/syscalls.c`の`call()`がシステムコール番号を`r0`，引数を`r1〜r3`に設定して`svc #0`を実行します．`isr_svcall`は例外フレームのアドレスを`os_svc_dispatch()`へ渡し，カーネルは結果を保存フレームの`r0`に反映します．

**PendSVはスケジューリング用の保留可能な例外**です．`kernel/kernel.c`の`pend()`はICSRの`PENDSVSET`ビットを設定します．`isr_systick()`は1 msごとに時刻を更新し，通常10 tickごと，またはスリープ中タスクが起きたときにPendSVを要求します．`SYS_YIELD`や`SYS_SLEEP`も必要に応じて`pend()`を呼びます．PendSVそのものは`BLOCKED`への状態変更を行いません．

SVCや割り込みの処理中に最低優先度のPendSVが要求されても，**通常は現在のハンドラが終わるまで保留**されます．カーネルの一時的なMSP使用を終えてから，次のタスクへ移るために役立ちます．

```text
タスクA（Thread，PSP A）
  └─ svc #0
      └─ isr_svcall（Handler，MSP）
          └─ os_svc_dispatch()
              └─ 待機状態を設定してpend()を要求
      └─ SVCの処理終了
  └─ isr_pendsv（Handler，MSP）
      └─ os_schedule() → PSP Bを選択
  └─ タスクB（Thread，PSP B）
```

### 4．`sleep()`とブロッキングI/O：カーネルスタックを残さない

PicoNIXの特徴は，**待機中のタスクのためにカーネル関数の呼び出し状態をMSP上へ残さない**ことです．ただし，CPUがMSPのメモリ領域を解放するわけではなく，ハンドラが終了してその領域を再利用できるようになります．

- **`SYS_SLEEP`**：`os_svc_dispatch()`が`task_t.wake`に起床時刻を保存し，`state=SLEEPING`にして`pend()`を要求します．`isr_systick()`が時刻を確認して`READY`へ戻します．この待機では，ユーザ側のSVC復帰状態を保持し，カーネル内の`sleep()`関数の途中から再開するわけではありません．
- **TTYのブロッキング`read()`**：`device_rw_tty()`は入力がなければ`wait_dev`，`wait_fd`，`wait_buf`，`wait_size`を保存し，`state=BLOCKED`にします．SVCハンドラは内部処理を終了しますが，ユーザタスクはまだ再開されません．
- **コンテキストの確定**：`os_schedule()`が旧PSPを`task_t.sp`に保存し，`wait_frame_ready=1`にします．保存フレームが完成する前に受信割り込みが来る場合もあるため，`wake_readers()`はこのフラグを確認します．
- **I/O完了**：`wake_readers()`がデータを保存された`wait_buf`へ転送し，`task_unblock(t, n)`を呼びます．`task_unblock()`は**`t->sp[8]`（保存済みの`r0`）**に読み取り結果を設定し，`state=READY`へ戻します．パイプの待機・復帰には`wake_pipes()`，子プロセス待ちには`wake_waiters()`も関わります．

```text
read(fd, buf, size) → SVC
  → 入力なし：BLOCKED，bufとsizeを保存，PendSVを要求
  → SVCのC関数は終了，共通MSPを再利用
  → 入力到着：bufへデータを転送し，保存済みr0へバイト数を設定
  → READYになったタスクが例外復帰
  → ユーザプログラムではread()が戻ったように見える
```

**データの保存先と戻り値の保存先を混同しないでください．** 読み取ったデータは`read()`に渡されたユーザバッファへ，バイト数はPSP側の保存済み`r0`へ書き込みます．そのため，バッファが有効なまま保持される必要があります．なお，デバイスごとの同期処理が短時間で完了するかどうかは別問題であり，すべてのI/Oが非同期に待機するわけではありません．

### 5．Research UNIXとの比較

Research UNIX V5/V6では，カーネル内の`sleep()`で待機したプロセスが，**プロセス固有のカーネルスタックに呼び出し状態を残したまま**切り替わり，起床後にその続きから処理を再開できます．カーネルが原則非プリエンプティブでも，カーネル内部で自発的にスケジュールアウトできるため，この仕組みが必要です．

対してPicoNIXは，待機条件とシステムコールの復帰状態を保存し，**カーネル関数の呼び出し状態を残さず**別タスクへ移ります．この違いは，プロセス別カーネルスタックの必要性と対応します．両方式とも「ユーザプログラムからはブロッキング`read()`に見える」という点は同じです．


## 関連する仕様と実装資料

- [関数一覧](../FUNCTIONS.md)：定義箇所と処理概要の索引
- [内部構造](../../reference/INTERNAL.md)：メモリ・スケジューラ・SVCの補足
- [システムコール一覧](../../reference/SYSCALLS.md)：番号，引数と動作
- [システムコール開発](../syscalls/SYSCALL-DEVELOPMENT.md)：システムコールの追加手順
- [カーネル拡張ガイド](KERNEL-DEVELOPMENT.md)：カーネルの拡張手順

---

**関連リンク：** [開発ガイド一覧](../README.md) ｜ [文書案内](../../README.md) ｜ [English](../../../en/development/kernel/KERNEL-READING-GUIDE.md)
