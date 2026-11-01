# PicoNIXのCアプリケーション開発

[English](../en/PROGRAMMING.md) | [文書案内](DOCS.md) | [Cライブラリ](LIBC.md)

この文書では，ホストPC上で**新しいCコマンドを作成し，ファームウェアに組み込んで実行するまで**を説明します．ツールチェーンの導入と実機の準備については，先に[セットアップ](SETUP.md)を参照してください．

PicoNIXのCアプリケーションは，Linuxの実行ファイルではありません．ホストPCで位置独立コード（PIC）としてビルドし，ファームウェアのFlashに組み込みます．起動時にイメージを空きRAMスロットへコピーして実行します．一方，カーネルのコードはFlash上でXIP実行されます．

## 1．既存のプログラムを確認する

PicoNIXを起動したら，次を試します．

```text
piconix# hello
Hello, PicoNIX!
piconix# hello Pico
Hello, Pico!
```

ソースは`userland/hello.c`です．通常のアプリは`main(int argc, char *argv[])`を定義し，`main()`から戻ると共通の`userland/crt0.c`が終了処理を行います．ホストPCのlibcやPico SDKのライブラリを直接リンクせず，PicoNIXのヘッダと小型Cライブラリを使用します．

## 2．新しい`greet.c`を作成する

**`userland/greet.c`**を新規作成し，次のコードを保存します．

```c
#include "stdio.h"

int main(int argc, char *argv[])
{
    const char *name = (argc > 1) ? argv[1] : "PicoNIX";
    printf("Hello, %s!\n", name);
    return 0;
}
```

`argv[0]`はコマンド名，`argv[1]`は指定された名前です．引数がなければ`PicoNIX`を使用します．`printf()`はPicoNIXの`stdio.h`で提供されます．関数の対応範囲は[Cライブラリ](LIBC.md)を参照してください．

## 3．ファームウェアにコマンドを登録する

現行のビルド構成では，**次の2か所を変更**します．

### 3.1．`CMakeLists.txt`へ追加

ファイル上部の`foreach(APP IN ITEMS ...)`に，拡張子`.c`を付けずに`greet`を加えます．

```cmake
foreach(APP IN ITEMS pins fbctl pwmctl kitesys shell blink omikuji python hello ... udp_echod greet)
```

上記の`...`は説明用の省略です．**実際には既存のアプリ名をすべて残して**，末尾などに`greet`を追加してください．この共通ループによって，ソースのコンパイル，共通の`crt0.c`・`syscalls.c`・`libc.c`のリンク，PIC検査，サイズ検査，Flashへのイメージ埋め込みが行われます．個別のビルドルールは不要です．

### 3.2．`kernel/main.c`へ登録

ほかのアプリの宣言と並べて，次の行を追加します．

```c
extern const unsigned char piconix_greet_image_start[], piconix_greet_image_end[];
```

さらに`main()`内の既存の`register_image(...)`と並べて登録します．

```c
register_image(39, "greet", piconix_greet_image_start, piconix_greet_image_end);
```

**39は，この例で照合したソースでは未使用のIDです．** 今後アプリが増えた場合は，登録済みIDを確認して空いている番号に変更してください．現在，プログラム登録IDは**1〜64**で，0は予約されています．`greet`という登録名が`/bin`に現れ，シェルから起動できます．シェル本体にコマンド専用の処理を追加する必要はありません．なお，`/tmp`に置いたC実行ファイルを動的にロードする方式ではありません．

## 4．ビルド・書き込み・実行

リポジトリのルートで，SDKとツールチェーンの設定後に次を実行します．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

Pico W向けの場合は`pico`を`pico_w`に変更します．SDKの配置が異なる場合，UF2の出力先や実機への書き込み方法は[セットアップ](SETUP.md)を確認してください．ビルド時にはPICとRAMスロットの検査も実行されます．UF2を書き込んだ後，次を試します．

```text
piconix# greet
Hello, PicoNIX!
piconix# greet Alice
Hello, Alice!
piconix# ls /bin
```

`greet`が見つからない場合は，CMakeのアプリ名，`register_image()`の登録，最新のUF2を書き込んだかを確認します．ホスト側のテストだけを実行しても，実機のプログラムは更新されません．

## 5．引数・FD・入出力

外部コマンドは`argc`と`argv`で引数を受け取ります．シェルは空白区切りで処理し，引用符で複数の単語をまとめることはできません．コマンド名を含めて**最大8文字列**，文字列領域は終端のNUL文字を含めて**合計128バイト**です．`argv[argc]`は`NULL`です．

標準FDは0が入力，1が出力，2がエラー出力です．標準出力に書く別の例は次のとおりです．

```c
#include "unistd.h"

int main(int argc, char *argv[])
{
    const char message[] = "hello\n";
    write(1, message, sizeof(message) - 1);
    return 0;
}
```

例えば`greet Alice > /tmp/message`とすると，出力先はRAMファイルになります．アプリは通常，リダイレクトやパイプを意識せず同じFDを使用できます．詳しくは[ファイルシステム](FILESYSTEM.md)，[パイプ](PIPE.md)，[デバイス](DEVICES.md)を参照してください．

## 6．利用できるAPIと実行上の制約

| 項目 | 現行仕様 |
|---|---|
| エントリポイント | `userland/crt0.c`経由の`main(argc, argv)` |
| RAMタスクスロット | 24 KiB × 6個 |
| スタック領域 | 各スロット末尾4 KiBを予約．イメージは残りの領域内に収める |
| アプリの配置 | PICイメージをFlashへ埋め込み，起動時にRAMへコピー |
| カーネルAPI | CラッパーからSVCシステムコールを使用 |
| タスク生成 | `spawn(program_id, argc, argv)`．空きスロットがなければ失敗 |
| 終了待ち | `waitpid()`はPOSIXの完全な実装ではない |

`userland/`以下の`stdio.h`，`stdlib.h`，`string.h`，`unistd.h`，`fcntl.h`を使用します．関数の引数・戻り値・対応範囲は[LIBC.md](LIBC.md)，低水準の呼び出し規約は[SYSCALLS.md](SYSCALLS.md)を参照してください．通常の`errno`を前提とせず，PicoNIXのエラー戻り値を確認します．

`fcntl()`による`O_NONBLOCK`だけでは，TTYのcanonical入力が文字単位にはなりません．対話的なプログラムについては[TTY](TTY.md)と`userland/mb.c`を参照してください．ネットワークプログラムには別の制約もあるため，[ネットワークプログラミング](NETWORK-PROGRAMMING.md)も参照してください．

## 7．問題が起きたとき

| 症状 | 確認すること |
|---|---|
| Cコンパイルエラー | `#include`，関数宣言，PicoNIX独自ヘッダの使い方 |
| 未定義シンボル（`__aeabi_*`を含む） | 未対応の依存関係，コンパイラ補助関数やリンク設定 |
| PIC・サイズ検査エラー | 位置依存コード，未対応の再配置，イメージ領域やスタック予約との重複 |
| `/bin`にコマンドがない | CMakeへの追加，シンボル宣言，`register_image()`，最新UF2の書き込み |
| 実行時に起動できない | タスクスロットの空きとプログラム登録 |
| 端末の動作がおかしい | canonical・echo設定と復元処理．[TTY](TTY.md)を参照 |

ホスト側の回帰テストは次のように実行します．

```sh
python3 tools/run_tests.py
python3 tools/run_tests.py --all  # ARMツールチェーン依存テストも実行
```

ソースレベルのテストだけでは，ファームウェアのビルドや実機動作までは保証できません．実行中の調査には`ps`，`cat /proc/tasks`，`trace on`，`trace off`，`trace show`などを使います．[トレース](TRACE.md)ではシステムコールとコンテキストスイッチ，[メモリ参照](MEM.md)ではRAM・Flashの読み取り専用観察を説明しています．

## 8．関連文書

- [Cライブラリ](LIBC.md)：アプリで使用できる関数と制限
- [システムコール](SYSCALLS.md)：番号，引数，戻り値，ABI
- [アセンブリプログラミング](ASM.md)・[Python](PYTHON.md)：PicoNIX上で作るプログラム（ホストでCをビルドする方式とは異なる）
- [全体構造](ARCHITECTURE.md)・[内部構造](INTERNAL.md)：PICロード，スケジューラ，カーネル実装
- [ネットワークプログラミング](NETWORK-PROGRAMMING.md)：Pico W上のTCP/UDPアプリ開発
