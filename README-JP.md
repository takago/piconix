# PicoNIX

[English](README-EN.md) | [文書一覧](#文書一覧)

PicoNIXは，**Raspberry Pi Pico（RP2040）向けの小型UNIX風OS**です．Linuxを移植したものではなく，タスク管理，システムコール，簡易ファイル，デバイスI/Oの仕組みを少ないコードで理解するための実験的なOSです．

## できること

- SysTick／PendSVによるプリエンプティブなタスク切り替え
- SVCによるカーネルとRAMアプリケーションの呼び出し境界
- `ps`，`kill`，`sh`，`getty`，`blink`，`omikuji`，`python`，`hello`，`hexdump`などのコマンド
- `&`による簡易バックグラウンド実行，`Ctrl+C`による前景タスクの中断
- `/dev`と`/proc`の仮想ファイル，`/tmp`の揮発性ファイル
- 標準入出力の簡易リダイレクト（`<`，`>`，`>>`）
- 整数演算やGPIOを扱える小型のPython風インタプリタ

## まず動かす

Pico SDK 2.1.1とARM GCCを導入したLinux環境で，次のようにビルドします．パスは環境に合わせて変更してください．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

生成した`build/piconix.uf2`をBOOTSELモードのPicoへコピーし，USB CDC端末に接続します．詳細は[セットアップ](SETUP-JP.md)を参照してください．

```text
piconix# help
piconix# ps
piconix# blink &
piconix# cat /proc/uptime
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# python
```

## 2つの端末で使う

USBコンソールのShellから`getty /dev/uart1 sh &`を実行すると，UART1に別のShellを起動できます．UART1側で起動したプログラムには同じ端末の標準入出力が継承されます．Python対話環境なら`getty /dev/uart1 python &`です．UART1は115200 bps，8N1で接続し，端末側のローカルエコーを利用してください．同一端末で複数の対話プログラムを競合させないため，占有中は新しい`getty`の起動に失敗します．

## 設計の特徴と制限

RP2040のRAMのうち，カーネルに96 KiB，アプリケーション用に32 KiBスロット5個を割り当てます．アプリケーションはPICとしてFlashに埋め込まれ，起動時に空きスロットで動作します．同時実行は最大5タスクです．

PicoNIXは完全なUNIX互換OSではありません．汎用シグナル配送，シグナルハンドラ，パイプ，プロセスごとのメモリ保護，ELF実行ファイルの動的読み込みは実装していません．`Ctrl+C`と`kill`は，UNIXのシグナル送信ではなく，カーネルによるタスクの直接終了です．`/tmp`の内容は再起動すると消えます．

## 文書一覧

| やりたいこと | 読む文書 |
|---|---|
| 必要な環境を準備し，書き込む | [セットアップ](SETUP-JP.md) |
| RAM配置，タスク，システムコールを理解する | [内部構造](INTERNAL-JP.md) |
| CでRAMアプリケーションを作る | [アプリ開発](PROGRAMMING-JP.md) |
| GPIO・ADC・PWMなどのデバイスを使用する | [デバイスガイド](DEVICES-JP.md) |
| UART1，`getty`，Ctrl+C／Ctrl+Dを理解する | [UARTと端末操作](UART-JP.md) |
| `python`コマンドを使う | [Python風インタプリタ](PYTHON-JP.md) |

## ライセンス

GPL-3.0-or-later．詳細は`LICENSE`を参照してください．

`/dev/mem`は読み出し専用です．`hexdump /dev/mem 0x20018000 64`でRAM，`hexdump /dev/mem 0x10000000 64`でFlashを確認できます．対象はRP2040のメインSRAM（256 KiB）と先頭2 MiBのXIP Flashのみです．周辺機器レジスタへのアクセスは許可しません．

### カーネルトレース（16 KiB）

`trace on` でログを初期化して記録を開始し，`trace off` で停止します．`trace show` は停止後にリングバッファを表示します（再表示可能）．`/dev/trace` は16バイト固定長のバイナリレコードを読み出す読み取り専用デバイスです．トレース制御は専用の `ioctl()` で行い，文字列整形は外部アプリ `trace.c` が担当します．

```text
trace on
hello
trace off
trace show
```

記録対象：コンテキストスイッチ，SVC開始・終了．タイムスタンプは起動後のSysTickミリ秒値です．

- `/bin`: 登録済み外部コマンドの読み出し専用一覧．`spawn` は最大8引数・合計128バイトの `argv[]` に対応．

トレースの詳細 / Trace details: [`TRACE-JP.md`](TRACE-JP.md) / [`TRACE-EN.md`](TRACE-EN.md)


## ps・cat・watch と割り込み表示

`ps` と `cat` は Shell 内蔵コマンドではなく，`/bin/ps`，`/bin/cat` のPICアプリです．`watch` も独立した `/bin/watch` です．

```text
ps
cat /proc/tasks
watch ps
watch 1 ps
watch 1 cat /proc/interrupts
watch 2 cat /proc/memory
```

`watch [秒] コマンド [引数...]` は外部プログラムを繰り返し実行します．間隔は1〜60秒，省略時は1秒です．ANSI制御文字によって表示を消去します．`watch` 自身と実行対象がそれぞれ1タスクを使用します．`ls` や `echo` は外部化されたため，`watch` から起動できます．現在のCtrl+Cはプロセスグループではなく，その端末のフォアグラウンドPIDだけを終了させます．`watch` 実行中にその子が残る可能性があるため，継続時間の短いコマンドを想定しています．

`/proc/interrupts` の `SysTick` は1 msごとの実割り込み回数です．`uart1` はUART1 IRQハンドラの実行回数です．UART0コンソール構成では `console` 行にもUART0 IRQ回数を表示します．USB CDCコンソール構成ではUSB割り込み回数を計測していないため `console` 行は出しません．`PendSV` と `SVC` は例外ハンドラのカウンタです．


## Shell コマンドの外部化と作業ディレクトリ

`pwd`，`ls`，`rm`，`echo`，`kill`，`tty` は `/bin` の独立PICアプリになりました．`pid` コマンドは削除しましたが，`getpid()` は残ります．Shell内蔵は `help`，`cd`，`exit`，`logout`，`getty` です．

カーネルはタスクごとにカレントディレクトリを保持し，`spawn()` で子へ継承します．`chdir()` は呼び出し元のカレントディレクトリを変更し，`getcwd()` はこれを取得します．`open()` と `unlink()` は相対パスを解決します．`cd /tmp` の後に `pwd`，`ls`，`cat name`，`rm name` を実行できます．

`ls /bin` はアプリの一覧に続いてShell内蔵コマンドを見出し付きで表示します．`watch 1 ls /tmp` と `watch 1 pwd` も使用できます．

### 1 msタイマーとADC

SysTickは1 ms，通常のタイムスライスは10 msです．`trace` の `TICKx1K` は1秒ごとに記録されます．`cat /dev/adc/0`～`2` でGPIO26～28のADC値（0～4095）を読み出せます．繰り返し測定は `watch 1 cat /dev/adc/0` などを使用します．

内蔵温度センサーは `cat /dev/adc/temp` で摂氏の概算値を表示します（基準電圧3.3 Vを仮定）．

### PWMとGPIO予約ピン（2026-10-27）

GP18/GP19は標準設定でPWM用（共通周波数は起動時1 kHz，Dutyは両方0%）です．`/dev/pwm/1/freq` は周波数（8～100000 Hz），`/dev/pwm/1/a`・`/dev/pwm/1/b` はDuty（0～100%）を十進数のテキストで読み書きできます．GPIO一覧からGP18，19，26～28を除外し，対応するGPIOパスも開けません．GP26～28はADC専用です．


### ピン割り当てとドライバ（2026-10）

`src/pins.h` の `GPn_MODE` を変更して再ビルドすると，`/dev/gpio`，`/dev/adc`，`/dev/pwm` の公開デバイスと初期化が連動します．ドライバは `src/gpio.c`，`src/adc.c`，`src/pwm.c` に分割し，カーネルのFD管理は `src/kernel.c` に残しています．PWM初期値は1 kHz，GP18/GP19ともDuty 0%です．USB CDCコンソールの標準構成ではGP0/1はGPIOとして使用でき，GP4/5はUART1用に予約しています．UART0コンソール構成ではGP0/1を`PIN_UART`に変更します．

### Pinout command

Run `pins` to display the Raspberry Pi Pico 40-pin header as an ASCII diagram (USB connector at top), with live GPIO direction/level, framebuffer ownership, UART, ADC, and PWM duty/frequency from existing GPIO/PWM devices. Labels are obtained dynamically from existing GPIO/PWM devices; framebuffer ownership and PWM settings reflect the current state.

### pwmctl

`pwmctl status` でPWM設定を表示します．`pwmctl freq 2000` はGP18/GP19共通周波数を2000 Hzに，`pwmctl a 50` はGP18のDutyを50%に設定します．`pwmctl b 25` も同様です．`pwmctl off` は両チャネルを0%にします．周波数は8～100000 Hz，Dutyは0～100%です．設定には既存の `/dev/pwm/1/*` を使用します．
