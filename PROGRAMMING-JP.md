# PicoNIXのCアプリケーション開発

[文書案内](DOCS-JP.md) | [概要](README-JP.md) | [English](PROGRAMMING-EN.md) | [内部構造](INTERNAL-JP.md)

この文書では，**既存アプリの変更から，新しいコマンドの登録・実行まで**を説明します．PicoNIXのアプリはLinux用の実行ファイルではなく，Flashに埋め込まれ，実行時にRAMへコピーされるPICプログラムです．

## 1．まず動かしてみる

ビルドとUF2の書き込みは[セットアップ](SETUP-JP.md)の手順に従ってください．起動したら，次のコマンドを試します．

```text
piconix# hello
Hello, PicoNIX!
piconix# hello Pico
Hello, Pico!
piconix# ls /bin
```

ソースを変更して動作の違いを確認したい場合は，`ram_apps/blink.c` の `msleep(500)` の値を変更して再ビルドします．GPIO25のLEDなど，実機の構成に応じて動作を確認してください．

## 2．最小のプログラム

`ram_apps/hello.c` は標準出力に表示する簡単な例です．

```c
#include "stdio.h"

int main(int argc, char *argv[]) {
    if (argc > 1)
        printf("Hello, %s!\n", argv[1]);
    else
        puts("Hello, PicoNIX!");
    return 0;
}
```

より小さい例では `unistd.h` の `write()` だけでも表示できます．通常のlibcやPico SDKに直接依存せず，PicoNIXが提供するヘッダと小型libcを利用してください．

```c
#include "unistd.h"

int main(int argc, char *argv[]) {
    const char msg[] = "hello\n";
    write(1, msg, sizeof(msg) - 1);
    return 0;
}
```

`main()` から戻ると，共通起動コード `ram_apps/crt0.c` が終了処理を行います．

## 3．新しいコマンドの追加

1. `ram_apps/` に新しいCファイルを作成します．まず `hello.c` を参考にするのが簡単です．
2. `CMakeLists.txt` にRAMアプリのビルド・PIC検査・Flash埋め込みの設定を追加します．既存のアプリ定義を参考にしてください．
3. `src/main.c` にイメージの開始・終了シンボル宣言と `register_image()` を追加します．**未使用のプログラムID**を割り当てます（登録容量は32エントリで，ID 0は予約）．
4. 全体を再ビルドし，PIC・スロットサイズ検証が通ることを確認します．
5. UF2を書き込んで，Shellから追加したコマンドを実行します．

Shell内にコマンド固有の処理を追加する必要はありません．カーネルが登録名とプログラムIDを管理し，Shellはその情報を利用します．実行ファイルを `/tmp` に置いて後からロードする方式ではありません．

## 4．RAM配置とビルド制約

| 項目 | 仕様 |
|---|---|
| エントリ | `main(int argc, char *argv[])`（`crt0.c` 経由） |
| タスクスロット | 32 KiB × 5個 |
| スタック予約 | 各スロット末尾4 KiB |
| プログラム | PICとしてビルドし，Flash内に埋め込み |
| 実行時 | 空きRAMスロットへコピー |
| 呼び出し | SVC経由でカーネルサービスを使用 |

`ram_apps/slot.ld` はコード・データとスタックの重複を禁止します．ビルドでは異なるRAM配置で生成したイメージの一致を検査し，GOTや未解決参照などPICとして問題のある構成を拒否します．一般的なホストlibcをそのままリンクすることはできません．

## 5．引数と戻り値

外部アプリの引数は `argc` と `argv` で受け取ります．Shellは空白区切りで解析し，引用符によるグループ化は行いません．コマンド名を含めて最大8文字列，文字列データは終端の `\0` を含め合計128バイト以内です．`argv[argc]` は `NULL` です．

`spawn(program_id, argc, argv)` でも起動できます．スロットがすべて使用中の場合，新しいプログラムは起動できません．`waitpid()` は簡易的な終了待ちであり，通常のPOSIXと同じ終了ステータス管理を提供しません．

## 6．よく使うAPI

| ヘッダ | 主な機能 |
|---|---|
| `unistd.h` | `read`，`write`，`open`，`close`，`msleep`，`spawn`，`waitpid` など |
| `fcntl.h` | `O_RDONLY`，`O_WRONLY`，`O_CREAT`，`O_NONBLOCK`，`F_GETFL`，`F_SETFL` |
| `stdio.h` | `printf`，`snprintf`，`puts`，`putchar`，`getchar`，`fgets` |
| `string.h` | 基本的な文字列・メモリ操作 |
| `stdlib.h` | `atoi`，`strtol`，`abs` など |

`printf` は `%s`，`%c`，`%d`，`%i`，`%u`，`%x`，`%X`，`%%` に対応しますが，幅・精度・浮動小数点には対応しません．`msleep()` の単位はミリ秒です．これらはPOSIXの完全実装ではありません．

## 7．ファイルとリダイレクト

Shellの `<`，`>`，`>>` は，標準入力・出力FDの設定として扱います．

```text
piconix# echo hello > /tmp/message
piconix# cat /tmp/message
hello
piconix# hexdump /tmp/message
```

アプリ側はリダイレクトを特別に意識せず，`read(0, ...)` と `write(1, ...)` を使用できます．`/tmp` は再起動で消えるRAMファイルです．`lseek()` は `/tmp` の通常ファイルと `/dev/mem` で `SEEK_SET` をサポートします．デバイスには通常ファイル用の `O_CREAT` や `O_TRUNC` を不用意に指定しないでください．

## 8．TTYとノンブロッキング入力

```c
#include "unistd.h"
#include "fcntl.h"

int flags = fcntl(0, F_GETFL, 0);
if (flags >= 0)
    fcntl(0, F_SETFL, flags | O_NONBLOCK);
/* 文字単位で読みたいときは，別途ioctlでcanonicalを解除する */
```

`O_NONBLOCK` はFDの動作を指定し，canonical/echoの設定とは別です．ゲーム `ram_apps/mb.c` は入力設定を一時変更して使用する例です．実際のモード復元方法と注意点は[端末設定と対話型入力](TTY-JP.md)を参照してください．

## 9．デバッグと検証

```sh
python3 tools/test_audit.py
python3 tools/test_ram_pipeline.py
```

これらの検査だけで全アプリのビルド・実機動作が保証されるわけではありません．ARM用ツールチェーンとPico SDKを使用したファームウェア全体のビルドが必要です．未解決の `__aeabi_*` 参照，32 KiBスロットの超過，PIC検証失敗，システムコール番号の不一致に注意してください．

実行時の観察には `ps`，`cat /proc/tasks`，`cat /proc/memory`，`trace on/off/show` が利用できます．詳しくは[内部構造](INTERNAL-JP.md)と[トレース](TRACE-JP.md)を参照してください．

## 10．開発時のチェックリスト

- [ ] プログラムIDが既存の登録と重複していない
- [ ] `main(argc, argv)` を実装している
- [ ] PicoNIXのユーザAPIだけを使っている
- [ ] PIC検証とサイズ検証が通る
- [ ] 5スロットの制約を考慮している
- [ ] 実機で引数・入出力・終了動作を確認した

**次に読む文書：** OS全体の実装は[内部構造](INTERNAL-JP.md)，デバイス固有の操作は[デバイスと配線](DEVICES-JP.md)を参照してください．
