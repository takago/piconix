# PicoNIXの内部構造

[文書案内](DOCS.md) | [概要](README.md) | [English](../en/INTERNAL.md) | [Cアプリ開発](PROGRAMMING.md)

この文書では，**PicoNIXがどのように動くか**を，起動からシステムコール，入出力まで順に説明します．実際にアプリを作る手順は[Cアプリ開発](PROGRAMMING.md)，配線は[デバイスと配線](DEVICES.md)を参照してください．

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
 └─ 32 KiBタスクスロット × 5
     └─ Shell / 外部コマンド（実行時にコピー）
           │ SVC
           ▼
         Kernel ── /dev，/proc，/tmp
```

LinuxのELF実行ファイルをロードする仕組み，MMUによるプロセス分離，動的リンク，汎用POSIXシグナルはありません．UNIXの基本概念を小さな実装で体験する設計です．

## 2．メモリとPICアプリ

| 開始アドレス | 領域 | 容量 |
|---|---|---:|
| `0x20000000` | カーネル用RAM領域 | 96 KiB |
| `0x20018000` | タスクスロット1 | 32 KiB |
| `0x20020000` | タスクスロット2 | 32 KiB |
| `0x20028000` | タスクスロット3 | 32 KiB |
| `0x20030000` | タスクスロット4 | 32 KiB |
| `0x20038000` | タスクスロット5 | 32 KiB |

各スロット末尾の4 KiBはスタック用に予約します．プログラム本体と静的データは，残りの領域に収まる必要があります．同時に利用できるタスクスロットは5個です（アイドル処理は別管理）．

アプリは位置独立コード（PIC）として作成し，Flashに保持します．起動時に空きスロットへイメージをコピーし，初期スタックを設定して実行します．`userland/slot.ld` が配置を定め，ビルド時に異なるリンクアドレスで生成したイメージを比較して位置独立性を検査します．詳細は[Cアプリ開発](PROGRAMMING.md)を参照してください．

## 3．起動とタスクの管理

1. `kernel/main.c` が `os_init()` を呼び，カーネルを初期化します．
2. Flashに埋め込まれた24個のアプリを，名前とプログラムIDで登録します．
3. Shell（プログラムID 2）を起動し，スケジューラへ制御を渡します．
4. 以後，SysTickとPendSVを使って実行タスクを切り替えます．実際の例外・コンテキスト処理は `kernel/context.S` にあります．

タスクの状態は `UNUSED`，`READY`，`SLEEPING`，`BLOCKED`，`DEAD` です．`spawn()` は登録済みプログラムを新しいスロットで実行し，`waitpid()` は子の終了を待ちます．`WNOHANG` ならブロックしません．終了通知は簡素化されており，終了ステータスは保持されません．

## 4．システムコールの境界

RAMアプリはカーネルの関数を直接呼ばず，`userland/syscalls.c` のラッパーを通してSVC命令を実行します．番号は `r0`，引数は `r1`～`r3`，結果は `r0` に置きます．カーネルのディスパッチは `kernel/kernel.c` にあります．

| 分野 | 主なAPI |
|---|---|
| タスク | `getpid()`，`spawn()`，`waitpid()`，`kill()`，`yield()`，`msleep()` |
| ファイル | `open()`，`read()`，`write()`，`close()`，`lseek()`，`unlink()`，`dup2()` |
| ディレクトリ | `chdir()`，`getcwd()` |
| 端末・制御 | `get_tty()`，`ioctl()`，`fcntl()`，`set_foreground()` |

システムコール番号はカーネルとユーザ側の両方で一致している必要があります．番号15は旧 `GETARG` の予約番号であり，新しい機能に流用しません．起動引数は初期スタックと `crt0.c` を介して `main(argc, argv)` に渡されます．

## 5．Shellとプロセスの関係

カーネルはタスクごとに作業ディレクトリ，FD，所属TTYを保持します．子タスクは必要な状態を継承します．Shellはコマンド行を解釈し，登録済みの `/bin` プログラムを起動します．`cd` など一部のみが組込みコマンドです．

```text
piconix# cd /tmp
piconix# echo hello > note
piconix# cat note
hello
piconix# ps
```

`>`，`>>`，`<` のリダイレクトはShell側で扱い，標準FD（0，1，2）を設定して子タスクを起動します．`dup2()` は簡易実装で，POSIXの共有ファイルオフセットは提供しません．パイプ，引用符による引数のグループ化，`2>` はありません．

`command &` は外部コマンドを待たずにShellへ戻る指定です．`Ctrl+C` は，カーネルがその端末の前景PIDを直接終了させる方式であり，POSIXシグナルやプロセスグループの配送ではありません．

## 6．TTYと入出力

USB CDCコンソールとUART1の端末を扱えます．`getty /dev/uart1 sh &` により別のShellを起動できます．端末ごとに所有Shellと前景PIDを管理し，Ctrl+Cはその端末の前景タスクに作用します．

端末の**入力モード**（canonical/非canonical，echo）は `ioctl()`，FDごとの**非ブロッキングフラグ**は `fcntl()` で設定します．この2種類は独立しています．詳しい操作例と `mb` は[端末設定と対話型入力](TTY.md)を参照してください．端末モードはPOSIX `termios` の完全実装ではなく，`VMIN` と `VTIME` はありません．

## 7．仮想ファイルとデバイス

| パス | 役割 |
|---|---|
| `/bin` | Flash埋め込みの登録プログラム一覧（読み取り専用） |
| `/tmp` | 再起動で消える簡易RAMファイル |
| `/dev/console`，`/dev/uart1` | 端末入出力 |
| `/dev/gpio/N`，`/dev/adc/*`，`/dev/pwm/*` | GPIO，ADC，PWM |
| `/dev/mem` | 診断用のメモリ読み取り |
| `/dev/trace` | バイナリトレースの読み取り |
| `/proc/tasks`，`/proc/uptime`，`/proc/interrupts`，`/proc/memory` | カーネル状態の参照 |

デバイス名とピンの割り当ては[デバイスと配線](DEVICES.md)にまとめています．FDはタスク単位で管理され，ファイル・デバイスを同じ `read()` / `write()` で操作できます．

## 8．トレースとデバッグ

カーネルには16バイト×1024件＝16 KiBのリングバッファがあり，タスク切り替え，SVC開始・終了などを記録します．`trace` コマンドが表示用の文字列に変換します．

```text
trace on
hello
trace off
trace show
```

`/dev/trace` はバイナリレコードを読み出すデバイスです．詳しいイベント定義と解釈は[トレース](TRACE.md)を参照してください．

## 9．ソースコードの読み方

| ファイル | 読むポイント |
|---|---|
| `kernel/main.c` | 起動，アプリ登録 |
| `kernel/kernel.c` | タスク，SVC，FD，`/proc`，`/tmp`，TTY |
| `kernel/context.S` | 例外とコンテキスト切り替え |
| `kernel/gpio.c`，`adc.c`，`pwm.c`，`fb.c` | 各デバイスの実装 |
| `kernel/pins.h` | 公開ピンと初期割り当て |
| `userland/shell.c` | コマンド解釈，リダイレクト |
| `userland/syscalls.c`，`crt0.c` | ユーザ空間のSVC呼び出しと起動 |
| `userland/slot.ld`，`CMakeLists.txt` | RAM配置，イメージ生成，PIC検証 |

**次に読む文書：** 実際にコマンドを追加する場合は[Cアプリ開発](PROGRAMMING.md)，端末の仕組みは[端末設定と対話型入力](TTY.md)，ハードウェアへの接続は[デバイスと配線](DEVICES.md)を参照してください．
