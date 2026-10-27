# Cアプリケーションの開発

[English](PROGRAMMING-EN.md) | [概要へ戻る](README-JP.md)

PicoNIXのアプリは通常のLinux実行ファイルではありません．**位置独立コード（PIC）として構築された，独立した小さなプログラム**です．まず既存のBlinkを改造すると仕組みを確認できます．

## 1．最初の演習：Blink

`ram_apps/blink.c`の`SYS_SLEEP`に渡す値（ミリ秒）を変更してください．例えば`500`を`250`にすると点滅間隔が短くなります．[セットアップ](SETUP-JP.md)のコマンドでビルドし，UF2を書き込んで`blink`を実行します．

## 2．RAMアプリの約束

| 項目 | 内容 |
|---|---|
| エントリポイント | `main(argc, argv)` |
| RAM配置 | 空きスロットを動的に選択（`0x20018000`から32 KiB刻みで5個） |
| サイズ | 各32 KiB（末尾4 KiBはスタック予約） |
| カーネル呼び出し | SVC（`r0`が番号，`r1`～`r3`が引数，戻り値は`r0`） |
| 使用できないもの | カーネルへの直接リンク，通常のlibcやSDK関数への依存 |

最小例は`ram_apps/blink.c`を参照してください．SVCの呼び方や番号を自己流に変更せず，既存の実装と`src/kernel.c`を突き合わせてください．

## 3．新しいコマンドの追加

1. `ram_apps/`に既存アプリと同じ方式のCファイルを作ります．
2. `CMakeLists.txt`にアプリのPICイメージのビルドと埋め込みを追加します．
3. `src/main.c`でプログラムを登録します．
4. `src/main.c`と`CMakeLists.txt`にアプリを登録します．Shellは`/bin`の登録情報から起動対象を検索します．
5. 同時実行数は最大5タスクです．空きスロットがない場合は起動できません．

現在はFlash埋め込みイメージ方式なので，実行ファイルを`/tmp`に保存して起動する機能はありません．

## 4．ファイル・標準入出力

`SYS_OPEN`，`SYS_READ`，`SYS_WRITE`，`SYS_CLOSE`でデバイスや`/tmp`ファイルにアクセスできます．Shellは`<`，`>`，`>>`を処理し，実行するタスクにFDを引き継ぎます．`dup2`はPOSIXと違い，共有オフセットを実現しません．デバイスを開く場合は通常ファイル用の`O_CREAT`／`O_TRUNC`を指定しないでください．

## 5．検証と注意点

```sh
python3 tools/test_audit.py
python3 tools/test_ram_pipeline.py
```

上記はホスト側検査です．RAMイメージのホスト検査対象はShellとBlinkであり，**全アプリの最終確認にはARM GCCとPico SDKによるビルドが必要**です．`__aeabi_*`のような未解決ランタイムシンボル，スロットのサイズ超過，システムコール番号の不一致に注意してください．

背景処理やシグナル未対応の詳細は[内部構造](INTERNAL-JP.md)を参照してください．

## 共通スタートアップ（crt0）

各RAMアプリには `ram_apps/crt0.c` がリンクされます．カーネルは初期スタックから `r0=argc`，`r1=argv` を渡し，`crt0.c` の `app_entry()` が `main(argc, argv)` を呼び出します．`main()` が戻ると `SYS_EXIT` で終了します．`SYS_GETARG` は削除され，番号15は予約のままです．

## ユーザアプリ用のAPI（第6段階：追加のみ）

`ram_apps/unistd.h`と`ram_apps/fcntl.h`がユーザ向けの関数・定数を宣言し，`ram_apps/syscalls.c`がSVCラッパーを提供します．すべてのRAMアプリに`syscalls.o`をリンクしますが，使用しない関数はセクションGCで除去します．

```c
#include "unistd.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {
    const char message[] = "hello\n";
    write(1, message, sizeof(message) - 1);
    return 0;
}
```

すべてのRAMアプリと`crt0.c`はユーザAPIを使用し，SVC命令は`syscalls.c`に集約しています．`sleep()`はPOSIXとは異なりミリ秒単位です．`unistd.h`と`fcntl.h`のAPIはPOSIXの完全な実装ではありません．


## ユーザ空間の小型Cライブラリ

`ram_apps/libc.c` は，ユーザプログラム用の最小限の標準C関数を提供します．
`stdio.h`（`printf`，`snprintf`，`sprintf`，`puts`，`putchar`，`getchar`，`fgets`），
`string.h`（文字列・メモリ操作），`stdlib.h`（`atoi`，`strtol`，`abs`）を使えます．

`printf`系の書式は `%s`，`%c`，`%d`，`%i`，`%u`，`%x`，`%X`，`%%` に限定しています．幅，精度，浮動小数点や`scanf`には対応していません．
`fgets`は`stdin`を含む簡易`FILE`を使用し，改行文字は保持します．
`puts`は末尾に改行を追加します．端末の表示改行が必要な場合は既存コード同様`\r\n`を明示してください．
このライブラリはカーネルのNewlibとは独立しており，ユーザ側の`read`／`write`システムコールだけに依存します．
ビルド時は未使用関数を削除し，全4アプリに対して異なるRAM配置でPIC検証します．

## 外部アプリの例

`ram_apps/hello.c`は`printf()`と`puts()`を使う最小例です．`hello`または`hello NAME`で実行します．`ram_apps/hexdump.c`は`read()`でファイルを読み込み，16進数とASCII文字で表示します．`hexdump /tmp/FILE`または`COMMAND > /tmp/FILE`後に利用できます．引数なしの`hexdump`は標準入力を読みます．どちらもROMに組み込まれたPICユーザプログラムで，タスクスロットにロードされます．

## /dev/mem と hexdump

`hexdump /dev/mem 0x20018000 64`はユーザスロット先頭64バイトを表示します．`hexdump /dev/mem 0x10000000 64`はFlashを表示します．`hexdump /tmp/test 0 32`のように通常ファイルもオフセットと長さを指定できます．`lseek(fd, offset, SEEK_SET)`は`/dev/mem`と`/tmp`の通常ファイルに対応します．メモリは読み出し専用で，カーネル内部の情報も見えるため教育・デバッグ用途に限定してください．

### カーネルトレース（16 KiB）

`trace on` でログを初期化して記録を開始し，`trace off` で停止します．`trace show` は停止後にリングバッファを表示します（再表示可能）．`/dev/trace` は16バイト固定長のバイナリレコードを読み出す読み取り専用デバイスです．トレース制御は専用の `ioctl()` で行い，文字列整形は外部アプリ `trace.c` が担当します．

```text
trace on
hello
trace off
trace show
```

記録対象：コンテキストスイッチ，SVC開始・終了．タイムスタンプは起動後のSysTickミリ秒値です．


## コマンド引数と /bin

外部アプリは `int main(int argc, char *argv[])` を使用します．`spawn(program_id, argc, argv)` により，コマンド名を含め最大8個の文字列を渡せます．文字列の合計は終端 `\0` を含め128バイト以内です．制限超過はエラーになります．`argv[argc]` は `NULL` です．Shellは空白で引数を区切ります（引用符によるグループ化は未対応）．

`/bin` は登録済みPICプログラムの読み出し専用仮想ディレクトリです．Shell の `ls /bin` は外部プログラムに加えて，`---- sh built-in commands ----` の見出しの下に内蔵コマンドも表示します．カーネルの `/bin` 自体には内蔵コマンドを追加していません．`ls /bin` で一覧表示し，`hello` と `/bin/hello` の両形式で実行できます．プログラムは引き続きFlashに保持され，起動時にRAMスロットへ展開されます．

```
hexdump /dev/mem 0x20018000 64
trace on
hello world
trace off
trace show
```

`trace show` は Time(ms)，PID，Command，Event，Detail の列形式で表示します．Commandはイベント記録時のプログラムIDから `/bin` の登録名を復元するため，終了したタスクも識別できます．コンテキストスイッチのPID・Commandは切替先を示します．バイナリレコードは引き続き16バイトです．

トレースの詳細 / Trace details: [`TRACE-JP.md`](TRACE-JP.md) / [`TRACE-EN.md`](TRACE-EN.md)


## waitpid / Shell built-ins

`waitpid(pid, status, 0)` は子の終了までブロックし，`WNOHANG` 指定時はブロックしません．終了時に子のタスク領域を即座に解放し，終了通知としてPIDのみを最大5件保持します．終了ステータスは保持しません．孤児の終了通知は保持しません．`ls /bin` は外部コマンドの後に `---- sh built-in commands ----` で区切って内蔵コマンドを表示します．


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

### ADCデバイス（RP2040）

`/dev/adc/0`，`/dev/adc/1`，`/dev/adc/2` はGPIO26～28の12ビットADCです．`/dev/adc/temp` はRP2040内蔵の温度センサーで，推定温度を摂氏（小数点以下1桁）で返します．読み取り専用で，`cat /dev/adc/0` は0～4095の値を十進数と改行で表示します．各open時に1回測定し，readするとEOFに達します．`watch 1 cat /dev/adc/0` で再測定できます．GPIO26～28はADC入力として初期化されるため，同時に通常GPIOとして使用しないでください．

`cat /dev/adc/temp` または `watch 1 cat /dev/adc/temp` で温度を読み取れます．内部温度センサーは校正済みの高精度温度計ではなく，基準電圧3.3 Vによる概算値です．

### PWMとGPIO予約ピン（2026-10-27）

GP18/GP19はPWM専用（共通周波数は起動時1 kHz，Dutyはそれぞれ50%/0%）です．`/dev/pwm/freq` は周波数（8～100000 Hz），`/dev/pwm/18`・`/dev/pwm/19` はDuty（0～100%）を十進数のテキストで読み書きできます．GPIO一覧からGP18，19，26～28を除外し，対応するGPIOパスも開けません．GP26～28はADC専用です．
