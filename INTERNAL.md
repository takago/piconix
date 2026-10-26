# PicoNIX 内部仕様（Stage 30）

この文書は，現在のソースコードの構成を理解し，学習・改造するための技術資料です．UNIXとの類似点だけでなく，**実装上の簡略化**も明記します．

## 1．全体構成

PicoNIXはPico SDKを使用して起動するカーネルと，独立した固定アドレスのRAMプログラムから構成されます．起動後はカーネルがタスクを管理し，Shellがコマンド入力を受け付けます．

```text
Flash: カーネル起動イメージ + 埋め込み済みRAMプログラム
                    │
                    ▼
             Kernel / Scheduler
                    │ SVCシステムコール
       ┌────────────┼────────────┐
       ▼            ▼            ▼
   Shell         Blink        Omikuji
   Bank 1        Bank 2        Bank 3
```

## 2．RAM配置

| 領域 | アドレス | サイズ | 用途 |
|---|---|---|---|
| Bank 0 | `0x20000000–0x2000FFFF` | 64 KiB | カーネル（SDKのRAM実行領域） |
| Bank 1 | `0x20010000–0x2001FFFF` | 64 KiB | Shell |
| Bank 2 | `0x20020000–0x2002FFFF` | 64 KiB | Blink |
| Bank 3 | `0x20030000–0x2003FFFF` | 64 KiB | Omikuji |

RP2040の全SRAMは264 KiBですが，上記の固定スロット設計は主に256 KiBを対象としています．RAMプログラムは`ram_apps/slot.ld`を利用して**スロットごとの固定アドレス**にリンクされます．スタック用に各スロット末尾の領域を予約し，`tools/check_slot_image.py`等で検査します．

現段階ではELFの動的ロードや再配置は行いません．プログラムのイメージはビルド時に生成され，ファームウェアに埋め込まれます．

## 3．起動とビルド

`src/main.c`が初期化とプログラム登録・起動を担当します．`src/kernel.c`がタスク管理，デバイス，システムコールを担当し，`src/context.S`が低レベルのコンテキスト切り替えを担います．

`tools/build_sdk.sh`は次を実行します．

1. `tools/test_audit.py`：リンク・配置に関するホスト側テスト．
2. `tools/test_ram_pipeline.py`：RAMイメージ生成パイプラインの検査．
3. Pico SDKを使用したCMake構成．
4. カーネル・RAMプログラムのビルドと配置検査．

SDKの`copy_to_ram`実行形式を使用し，カーネル用RAMをBank 0に制限したリンカスクリプトを生成します．成果物名は現時点では`piconix.uf2`です．

## 4．タスクとスケジューラ

カーネルはタスクごとにPID，状態，スタックポインタ，名前，ファイル記述子などを保持します．`OS_MAX_TASKS`は3で，これとは別にアイドルタスクがあります．

状態は`UNUSED`，`READY`，`SLEEPING`，`DEAD`，`BLOCKED`です．`ps`では主に実行可能・スリープ・待機状態を確認できます．SysTickは時刻管理とスケジューリングに使用され，PendSVがコンテキスト切り替えに使用されます．`sleep`や入出力待機ではCPUを他タスクに譲れます．

### 終了と`waitpid`

タスク終了時には実行を止め，PID・親PID・終了ステータスをカーネル内の終了記録配列に保持します．実行スロットの状態とは別に終了結果を保存するため，親が後から結果を取得できます．

`SYS_WAITPID`は，終了済みの子の記録があればPIDとステータスを返し，子がまだ動作中なら`0`を返す**非ブロッキングのカーネルプリミティブ**です．ユーザ向け`os_waitpid()`ラッパーはスリープを挟んで繰り返し確認します．UNIXの本格的なブロッキング待機や完全なゾンビ管理と同一ではありません．

終了記録は有限個（`OS_MAX_TASKS`件）で，現段階では本格的な孤児プロセス回収・ジョブテーブルを備えていません．

## 5．システムコール

外部RAMプログラムは，レジスタ`r0`にシステムコール番号，`r1`〜`r3`に引数を置き，`svc #0`を実行します．戻り値は`r0`で受け取ります．主な番号は以下のとおりです．

| 番号 | 名称 | 役割 |
|---|---|---|
| 1 | `GETPID` | 自身のPID |
| 2 | `KILL` | 指定タスクの終了 |
| 3 | `EXIT` | 自タスクの終了 |
| 4 | `SLEEP` | 指定時間の休止 |
| 5 | `YIELD` | CPUを譲る |
| 6–10 | `OPEN` / `CLOSE` / `READ` / `WRITE` / `IOCTL` | デバイス・ファイル風入出力 |
| 11 | （予約済み） | 旧`TASK_INFO`（廃止） |
| 12 | `SPAWN` | 登録済みプログラムから新しいタスクを生成して起動する |
| 13 | `WAITPID` | 子タスクの終了記録を取得 |
| 14 | `SET_FG` | フォアグラウンドPIDの指定 |
| 15 | `GETARG` | Omikuji向けの簡易起動引数 |

プログラム起動用システムコールは`SYS_SPAWN`（番号12）です．C APIは`os_spawn_program(program_id)`で，既存のエントリ関数指定型`os_spawn(entry)`／`os_spawn_named(entry, name)`とは区別します．Shellの`blink`や`omikuji`コマンドは`SYS_SPAWN`を呼び出します．

システムコール境界ではユーザ側のポインタがタスク領域に属するかなどを検査します．ただし，現時点で完全なメモリ保護機構があるわけではありません．

## 6．フォアグラウンド／バックグラウンド

Shellは`blink`や`omikuji`を起動した後，末尾に`&`がなければ`waitpid`で終了を待ちます．`&`があれば待たずにプロンプトへ戻ります．`run`はShellコマンドとしては削除済みです．

```text
piconix# blink       ← 前景：Shellは終了待ち，Ctrl+Cで中断
piconix# blink &     ← 背景：Shellは入力を受け付け続ける
piconix# omikuji     ← 前景：標準入力から秒数を取得
piconix# omikuji 5 & ← 背景：標準入力不要
```

端末の入力権は**前景タスク**にあります．前景子タスクがいない場合はShellが入力権を持ちます．カーネルは`foreground_pid`でこれを判定し，背景タスクがUSBコンソールを`read(0, ...)`しようとするとエラーを返します．これはUNIXのプロセスグループと`SIGTTIN`を簡略化した設計です．Ctrl+Cは前景タスクを終了させます．

現状，バックグラウンド出力とShellの入力行を整形・再描画する機構はありません．

## 7．Omikujiの入出力

`omikuji`は前景では`read(0, ...)`で1〜30秒の入力を受け取ります．`omikuji N`は引数相当の値を`GETARG`から取得します．`omikuji &`は標準入力を読まず既定の10秒で動作します．

一般的な`argc/argv`はまだありません．乱数も暗号学的な乱数ではなく，カーネルのtick値から作る簡易的な疑似乱数です．

## 8．デバイスと仮想ファイル

| パス | 機能 |
|---|---|
| `/dev/console` | コンソール |
| `/dev/uart1` | UART1（GP4=TX，GP5=RX） |
| `/dev/rand` | 読み取りごとに32ビットの疑似乱数を1個返す（4バイト固定，暗号用途不可） |
| `/dev/gpio/N` | GPIO値の読み書き |
| `/dev/gpio/N.dir` | GPIO方向（`in`／`out`） |
| `/dev/gpio/N.pull` | プルアップ／プルダウン設定 |
| `/sys/proc/uptime` | 稼働時間 |
| `/sys/proc/ticks` | tick数 |
| `/sys/proc/interrupts` | 割り込み統計 |
| `/sys/proc/context_switches` | コンテキスト切り替え統計 |
| `/sys/proc/memory` | メモリ・スロット配置情報 |
| `/sys/proc/tasks` | 実行中タスクのPID・状態・スロット・保存SP（`ps`の情報源） |

通常のディスク上のファイルシステムではなく，カーネルがパスを解釈する仮想的な入出力インターフェースです．UART1の`cat`には約200 msの無通信終了条件があります．

## 9．現在の限界と今後の拡張候補

- 真のブロッキング`waitpid`と終了イベントによるShellの起床
- 汎用`argc/argv`と外部コマンドローダ
- 標準入出力のリダイレクト，パイプ，端末の行再描画
- プロセスグループ，シグナル，`fg`／`bg`コマンド
- システムコールABIの整理とユーザ空間保護

## 10．関連ファイル

- [`src/kernel.c`](src/kernel.c) — タスク，システムコール，デバイス
- [`src/context.S`](src/context.S) — コンテキスト切り替え
- [`src/main.c`](src/main.c) — 初期化・起動
- [`include/piconix.h`](include/piconix.h) — API定義
- [`ram_apps/shell.c`](ram_apps/shell.c) — Shell
- [`ram_apps/blink.c`](ram_apps/blink.c) — LED点滅
- [`ram_apps/omikuji.c`](ram_apps/omikuji.c) — カウントダウンと入力
- [`ram_apps/slot.ld`](ram_apps/slot.ld) — 固定RAMリンク

### 起動バナーとUSB CDC接続

Shellは `SYS_CONSOLE_READY`（SVC番号16）でUSB CDCの接続状態を確認し，接続前には10msのスリープを繰り返します．接続後にバナーと最初のプロンプトを表示します．UARTコンソール構成では即座に表示します．この待機はShellタスクのみで，スケジューラとアイドルタスクは動作を続けます．

### `/dev` registry and USB output

`src/kernel.c` defines `dev_registry[]` as the common source for fixed device
paths accepted by `open()` and reported by `/sys/proc/devices`. `ls /dev`
reads this virtual file; the GPIO subtree is generated separately.

USB CDC output from `SYS_WRITE` is queued in a bounded 512-byte kernel ring.
`SYS_WRITE` may return a short count (including zero) when the queue is full.
The idle task drains the queue to the SDK USB stdio driver only when the
TinyUSB CDC transmit FIFO has room; this avoids waiting for USB IRQ progress
inside the SVC exception. RAM applications retry short writes and sleep briefly
when the queue is full. Output is discarded on USB disconnection.

### Shellのカレントディレクトリ

`pwd`，`cd`，`ls`の引数省略，および`cat`と`echo`の相対パスは`ram_apps/shell.c`で実装しています．Shellはローカル変数としてカレントディレクトリを保持し，`.`・`..`を正規化した絶対パスを既存のシステムコールへ渡します．カーネルやほかのタスクにカレントディレクトリ状態は追加していません．実ファイルシステムではなく，既存の仮想ディレクトリに限定した簡易仕様です．
