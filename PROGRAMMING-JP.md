# Cアプリケーションの開発

[English](PROGRAMMING-EN.md) | [概要へ戻る](README-JP.md)

PicoNIXのアプリは通常のLinux実行ファイルではありません．**位置独立コード（PIC）として構築された，独立した小さなプログラム**です．まず既存のBlinkを改造すると仕組みを確認できます．

## 1．最初の演習：Blink

`ram_apps/blink.c`の`SYS_SLEEP`に渡す値（ミリ秒）を変更してください．例えば`500`を`250`にすると点滅間隔が短くなります．[セットアップ](SETUP-JP.md)のコマンドでビルドし，UF2を書き込んで`blink`を実行します．

## 2．RAMアプリの約束

| 項目 | 内容 |
|---|---|
| エントリポイント | `main(argc, argv)` |
| RAM配置 | 空きスロットを動的に選択（`0x20010000`から32 KiB刻みで6個） |
| サイズ | 各32 KiB（末尾4 KiBはスタック予約） |
| カーネル呼び出し | SVC（`r0`が番号，`r1`～`r3`が引数，戻り値は`r0`） |
| 使用できないもの | カーネルへの直接リンク，通常のlibcやSDK関数への依存 |

最小例は`ram_apps/blink.c`を参照してください．SVCの呼び方や番号を自己流に変更せず，既存の実装と`src/kernel.c`を突き合わせてください．

## 3．新しいコマンドの追加

1. `ram_apps/`に既存アプリと同じ方式のCファイルを作ります．
2. `CMakeLists.txt`にアプリのPICイメージのビルドと埋め込みを追加します．
3. `src/main.c`でプログラムを登録します．
4. `ram_apps/shell.c`にコマンドの起動処理を追加します．
5. 同時実行数は最大6タスクです．空きスロットがない場合は起動できません．

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

int main(int argc, char **argv) {
    const char message[] = "hello\n";
    write(1, message, sizeof(message) - 1);
    return 0;
}
```

すべてのRAMアプリと`crt0.c`はユーザAPIを使用し，SVC命令は`syscalls.c`に集約しています．`sleep()`はPOSIXとは異なりミリ秒単位です．`unistd.h`と`fcntl.h`のAPIはPOSIXの完全な実装ではありません．
