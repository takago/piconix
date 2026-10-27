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
4. `ram_apps/shell.c`にコマンドの起動処理を追加します．
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

`spawn(program, value, argument)` の `argument` は，全アプリ共通で `argv[1]` として渡されます（最大30バイトの1文字列）．現在の仕組みは一般的な複数引数の解析には対応していません．`value` は従来のOmikuji用の数値指定を維持します．
