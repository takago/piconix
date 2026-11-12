# PicoNIXの内部構造

*技術リファレンス*

この文書では，**PicoNIXがどのように動くか**を，起動からシステムコール，入出力まで順に説明します．実際にアプリを作る手順は[Cアプリ開発](../development/apps/c/PROGRAMMING.md)，配線は[ハードウェア配線](hardware/WIRING.md)，`/dev`の操作は[デバイス仕様](devices/README.md)を参照してください．

## 1．全体像

PicoNIXはRP2040上で動作する小さなUNIX風OSです．カーネルはスケジューリング，タスク管理，仮想ファイル，入出力を担当し，Shellとコマンドは独立したRAMアプリとして実行します．

```text
Flash
 ├─ Kernel + Pico SDK
 └─ 登録済みアプリのPICイメージ（/binの実体）
                  │ spawn
                  ▼
RAM
 ├─ Kernel領域
 └─ 24 KiBタスクスロット × 6
     └─ Shell / 外部コマンド（実行時にコピー）
           │ SVC
           ▼
         Kernel ── /dev，/proc，/tmp
```

LinuxのELF実行ファイルをロードする仕組み，MMUによるプロセス分離，動的リンク，汎用POSIXシグナルはありません．UNIXの基本概念を小さな実装で体験する設計です．

## 2．メモリとPICアプリ

| 開始アドレス | 領域 | 容量 |
|---|---|---:|
| `0x20000000` | カーネル用RAM領域 | 56 KiB |
| `0x2000E000` | タスクスロット1 | 24 KiB |
| `0x20014000` | タスクスロット2 | 24 KiB |
| `0x2001A000` | タスクスロット3 | 24 KiB |
| `0x20020000` | タスクスロット4 | 24 KiB |
| `0x20026000` | タスクスロット5 | 24 KiB |
| `0x2002C000` | タスクスロット6 | 24 KiB |
| `0x20032000` | 共有メモリ（SHM） | 2 KiB |
| `0x20032800` | RAMファイルのデータ領域（11 × 2 KiB） | 22 KiB |
| `0x20038000` | トレースリング専用領域 | 16 KiB |
| `0x2003C000` | ネットワーク予約領域 | 16 KiB |

各スロット末尾の4 KiBをスタック用に予約し，プログラム本体と静的データには最大20 KiBを利用できます．同時に利用できるタスクスロットは6個です（アイドル処理は別管理）．

アプリは位置独立コード（PIC）として作成し，Flashに保持します．起動時に空きスロットへイメージをコピーし，初期スタックを設定して実行します．`userland/lib/slot.ld` が配置を定め，ビルド時に異なるリンクアドレスで生成したイメージを比較して位置独立性を検査します．詳細は[Cアプリ開発](../development/apps/c/PROGRAMMING.md)を参照してください．

## 3．起動とタスクの管理

1. `kernel/main.c` が `os_init()` を呼び，カーネルを初期化します．
2. Flashに埋め込まれたアプリを，名前とプログラムIDで登録します．
3. Shell（プログラムID 2）を起動し，スケジューラへ制御を渡します．
4. 以後，SysTickとPendSVを使って実行タスクを切り替えます．実際の例外・コンテキスト処理は `kernel/context.S` にあります．

タスクの状態は `UNUSED`，`READY`，`SLEEPING`，`BLOCKED`，`DEAD` です．`spawn()` は登録済みプログラムを新しいスロットで実行し，`waitpid()` は子の終了を待ちます．`WNOHANG` ならブロックしません．終了通知は簡素化されており，終了コード（0〜255）をタスクスロットとは別の終了通知に保持し，`waitpid()`で親へ渡します．

`ps`は80桁端末向けに`PID`，`NAME`，`STATE`，`TTY`，`SLOT`，`PPID`，`DISPATCH`，`CPU_TICKS`を整列表示します．`/proc/tasks`は従来の詳細形式（`MEMORY`，`SP`，`NET_US`，`CON_US`を含む）を維持し，`top`などの解析元として利用します．どちらもPID 0の`[idle]`行を含みます．

## 4．システムコールの境界

RAMアプリはカーネルの関数を直接呼ばず，`userland/lib/syscalls.c` のラッパーを通してSVC命令を実行します．番号は `r0`，引数は `r1`～`r3`，結果は `r0` に置きます．カーネルのディスパッチは `kernel/kernel.c` にあります．

| 分野 | 主なAPI |
|---|---|
| タスク | `getpid()`，`spawn()`，`waitpid()`，`kill()`，`yield()`，`msleep()` |
| ファイル | `open()`，`read()`，`write()`，`close()`，`lseek()`，`unlink()`，`dup2()` |
| ディレクトリ | `chdir()`，`getcwd()` |
| 端末・制御 | `get_tty()`，`ioctl()`，`fcntl()`，`set_foreground()` |

システムコール番号はカーネルとユーザ側の両方で一致している必要があります．システムコール番号15は予約済みで，割り当てません．起動引数は初期スタックと `crt0.c` を介して `main(argc, argv)` に渡されます．

詳細な番号一覧は[システムコール一覧](SYSCALLS.md)を参照してください．

## 5．Shellとプロセスの関係

カーネルはタスクごとに作業ディレクトリ，FD，所属TTYを保持します．子タスクは必要な状態を継承します．Shellはコマンド行を解釈し，登録済みの `/bin` プログラムを起動します．`cd` など一部のみが組込みコマンドです．

```text
piconix# cd /tmp
piconix# echo hello > note
piconix# cat note
hello
piconix# ps
```

`>`，`>>`，`<` のリダイレクトはShell側で扱い，標準FD（0，1，2）を設定して子タスクを起動します．`dup2()` は簡易実装で，POSIXの共有ファイルオフセットは提供しません．最大3段のパイプライン（`|`）に対応します．引用符による引数のグループ化と`2>`はありません．

子プロセスが継承するのは標準FD（0～2）のみです．パイプ端を継承・複製する際は参照数を増やし，`close()`やタスク終了時に減らします．別のTTYへ起動する`getty`は，継承した標準FDを解放してからTTYへ付け替えます．TCP/UDPソケットの`dup()`/`dup2()`はサポートしません．

`command &` は外部コマンドを待たずにShellへ戻る指定です．`Ctrl+C` は，カーネルがその端末の前景に登録されたタスク（パイプラインの場合は最大3タスク）を直接終了させる方式であり，POSIXシグナルやプロセスグループの配送ではありません．

タスクが終了すると，カーネルはFDとソケットを解放し，前景タスクの登録から終了済みPIDを除外します．パイプラインの途中の段が先に終了したときは，残っている段がTTYの前景所有権とTTY設定の復元責務を引き継ぎます．`waitpid()`に渡す終了通知は再利用可能なタスクスロットとは別に保持されます（終了コード（0〜255）も返します）．PID 0はアイドルタスク用で，通常タスクのPIDは1〜255を循環使用します．割り当て時は稼働中のタスクと未回収の終了通知を照合して重複を避けます．端末入力・パイプ入出力・`waitpid()`のブロックを解除する際は，保存済みのSVC復帰値を書き換えた後，待機情報と一時的なユーザポインタを消去してからタスクをREADYに戻します．

## 6．TTYと入出力

USB CDCではCtrl+Tで切り替える2つの独立TTY（`/dev/ttyV0`と`/dev/ttyV1`）を扱い，UART1も独立しています．`getty /dev/ttyS1 sh &` により別のShellを起動できます．端末ごとに所有Shellと前景PIDを管理し，Ctrl+Cはその端末の前景タスクに作用します．

端末の**入力モード**（canonical/非canonical，echo）は `ioctl()`，FDごとの**非ブロッキングフラグ**は `fcntl()` で設定します．この2種類は独立しています．詳しい操作例と `mb` は[端末設定と対話型入力](commands/TTY.md)を参照してください．端末モードはPOSIX `termios` の完全実装ではなく，`VMIN` と `VTIME` はありません．

## 7．仮想ファイルとデバイス

| パス | 役割 |
|---|---|
| `/bin` | Flash埋め込みの登録プログラム一覧（読み取り専用） |
| `/tmp` | 再起動で消える簡易RAMファイル |
| `/dev/ttyV0`，`/dev/ttyS1` | 端末入出力 |
| `/dev/gpio/N`，`/dev/adc/*`，`/dev/pwm/*` | GPIO，ADC，PWM |
| `/dev/mem` | 許可範囲内のメモリ読み書き |
| `/dev/trace` | バイナリトレースの読み取り |
| `/proc/tasks`，`/proc/uptime`，`/proc/interrupts`，`/proc/memory`，`/proc/cpu` | カーネル状態の参照 |

デバイス名は[デバイス仕様](devices/README.md)，ピンの割当は[ピン割当](hardware/PINS.md)，接続例は[配線](hardware/WIRING.md)にまとめています．FDはタスク単位で管理され，ファイル・デバイスを同じ `read()` / `write()` で操作できます．

内部では，`/dev/gpio/N`，`/dev/gpio/N.dir`，`/dev/gpio/N.pull`の番号解析を`open()`と`getdents()`で共通化しています．ピンの利用可否は`kernel/pins.h`で判断します．相対パスの正規化では，ユーザ文字列の各文字を参照する前にタスクRAMの範囲を確認します．未対応のデバイスIDをGPIOに振り分けず，読み書きできない場合はエラーを返します．なお，`/dev/i2c/1`の転送は`read()`・`write()`ではなく`ioctl()`で行います．

## 8．トレースとデバッグ

カーネルは`0x20038000`〜`0x2003BFFF`に置いた32バイト×512件＝16 KiBのリングバッファを管理し，タスク切り替え，SVC開始・終了などを記録します．`trace` コマンドが表示用の文字列に変換します．SysTick，UART1，SVCから同時に記録されないよう，1レコードの書込みとリング管理値の更新を短い割り込み禁止区間で行います．

```text
trace on -s
hello
trace off
trace show
```

標準ビルドは関数計測ONですが，起動時のイベント記録はOFFです．`trace on -s`はシステムコールの入口・出口だけを記録し，通常の`trace on`は選抜カーネル関数やTIMER・SWITCHも記録します．`trace on -a`ではSVC外の選抜関数も記録します．

`/dev/trace` はバイナリレコードを読み出すデバイスです．詳しいイベント定義と解釈は[トレース](commands/TRACE.md)を参照してください．

## 9．実装の参照先

| ファイル | 読むポイント |
|---|---|
| `kernel/main.c` | 起動，アプリ登録 |
| `kernel/kernel.c` | タスク，SVC，FD，`/proc`，`/tmp`，TTY |
| `kernel/context.S` | 例外とコンテキスト切り替え |
| `kernel/gpio.c`，`adc.c`，`pwm.c`，`fb.c` | 各デバイスの実装 |
| `kernel/pins.h` | 公開ピンと初期割り当て |
| `userland/apps/shell.c` | コマンド解釈，リダイレクト |
| `userland/lib/syscalls.c`，`crt0.c` | ユーザ空間のSVC呼び出しと起動 |
| `userland/lib/slot.ld`，`CMakeLists.txt` | RAM配置，イメージ生成，PIC検証 |

**次に読む文書：** 実際にコマンドを追加する場合は[Cアプリ開発](../development/apps/c/PROGRAMMING.md)，端末の仕組みは[端末設定と対話型入力](commands/TTY.md)，ハードウェアへの接続は[配線ガイド](hardware/WIRING.md)を参照してください．

Pico Wではネットワーク予約16 KiBにlwIPヒープ10 KiBとアイドルスタック4 KiBを配置します．[RAMレイアウト](MEMORY-LAYOUT.md)を参照してください．

---

**関連リンク：** [リファレンス目次](README.md) ｜ [文書案内](../README.md) ｜ [操作ガイド一覧](commands/README.md) ｜ [English](../../en/reference/INTERNAL.md)
