# PicoNIXのCアプリケーション開発

## 読み始める前に

C言語の関数，配列，コマンドライン引数を復習してから始めてください．最初は既存の`hello`を実行し，次に`greet.c`を追加します．編集するソースはPC側にあり，反映にはUF2の再書き込みが必要です．


[English](../../../../en/development/apps/c/PROGRAMMING.md) | [文書案内](../../../../DOCS.ja.md) | [Cライブラリ](LIBC.md)

この文書では，ホストPC上で**新しいCコマンドを作成し，ファームウェアに組み込んで実行するまで**を説明します．ツールチェーンの導入と実機の準備については，先に[セットアップ](../../../setup/SETUP.md)を参照してください．

PicoNIXのCアプリケーションは，Linuxの実行ファイルではありません．ホストPCで位置独立コード（PIC）としてビルドし，ファームウェアのFlashに組み込みます．起動時にイメージを空きRAMスロットへコピーして実行します．一方，カーネルのコードはFlash上でXIP実行されます．

## 1．既存のプログラムを確認する

PicoNIXを起動したら，次を試します．

```text
piconix# hello
Hello, PicoNIX!
piconix# hello Pico
Hello, Pico!
```

ソースは`userland/apps/hello.c`です．通常のアプリは`main(int argc, char *argv[])`を定義し，`main()`から戻ると共通の`userland/lib/crt0.c`が終了処理を行います．ホストPCのlibcやPico SDKのライブラリを直接リンクせず，PicoNIXのヘッダと小型Cライブラリを使用します．

## 2．新しい`greet.c`を作成する

**`userland/apps/greet.c`**を新規作成し，次のコードを保存します．

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
foreach(APP IN ITEMS net pins i2cctl bh1750 fbctl pwmctl kitesys shell blink omikuji python hello hexdump trace ps cat grep watch pwd ls rm echo kill tty stty mb reset lsof pong gpio fbed asm help edit vi dd mem tree find nc httpd tcp_echod udp_echod greet)
```

上記は現在の`CMakeLists.txt`に`greet`を末尾へ追加した例です．既存のアプリ名は削除せず，新しい版で一覧が変わっている場合はその版の末尾へ`greet`を追加してください．この共通ループによって，ソースのコンパイル，共通の`crt0.c`・`syscalls.c`・`libc.c`のリンク，PIC検査，サイズ検査，Flashへのイメージ埋め込みが行われます．個別のビルドルールは不要です．

### 3.2．`kernel/main.c`へ登録

ほかのアプリの宣言と並べて，次の行を追加します．

```c
extern const unsigned char piconix_greet_image_start[], piconix_greet_image_end[];
```

さらに`main()`内の既存の`register_image(...)`と並べて登録します．

```c
register_image(44, "greet", piconix_greet_image_start, piconix_greet_image_end);
```

**44は，この添付版で未使用のプログラム登録IDです（39は`i2cctl`，40は`bh1750`，41は`vi`，42は`dd`，43は`net`が使用）．** 今後アプリが増えた場合は，登録済みIDを確認して空いている番号に変更してください．現在，プログラム登録IDは**1〜64**で，0は予約されています．`greet`という登録名が`/bin`に現れ，シェルから起動できます．シェル本体にコマンド専用の処理を追加する必要はありません．なお，`/tmp`に置いたC実行ファイルを動的にロードする方式ではありません．

## 4．ビルド・書き込み・実行

リポジトリのルートで，SDKとツールチェーンの設定後に次を実行します．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

Pico W向けの場合は`pico`を`pico_w`に変更します．SDKの配置が異なる場合，UF2の出力先や実機への書き込み方法は[セットアップ](../../../setup/SETUP.md)を確認してください．ビルド時にはPICとRAMスロットの検査も実行されます．UF2を書き込んだ後，次を試します．

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

例えば`greet Alice > /tmp/message`とすると，出力先はRAMファイルになります．アプリは通常，リダイレクトやパイプを意識せず同じFDを使用できます．詳しくは[ファイルシステム](../../../reference/FILESYSTEM.md)，[パイプ](../../../literacy/PIPE.md)，[デバイス](../../../reference/devices/DEVICES.md)を参照してください．

## 6．利用できるAPIと実行上の制約

| 項目 | 現行仕様 |
|---|---|
| エントリポイント | `userland/lib/crt0.c`経由の`main(argc, argv)` |
| RAMタスクスロット | 24 KiB × 6個 |
| スタック領域 | 各スロット末尾4 KiBを予約．イメージは残りの領域内に収める |
| アプリの配置 | PICイメージをFlashへ埋め込み，起動時にRAMへコピー |
| カーネルAPI | CラッパーからSVCシステムコールを使用 |
| タスク生成 | `spawn(program_id, argc, argv)`．空きスロットがなければ失敗 |
| 終了待ち | `waitpid()`はPOSIXの完全な実装ではない |

`userland/`以下の`stdio.h`，`stdlib.h`，`string.h`，`unistd.h`，`fcntl.h`を使用します．関数の引数・戻り値・対応範囲は[LIBC.md](LIBC.md)，低水準の呼び出し規約は[SYSCALLS.md](../../../reference/SYSCALLS.md)を参照してください．通常の`errno`を前提とせず，PicoNIXのエラー戻り値を確認します．

`fcntl()`による`O_NONBLOCK`だけでは，TTYのcanonical入力が文字単位にはなりません．対話的なプログラムについては[TTY](../../../literacy/TTY.md)と`userland/apps/mb.c`を参照してください．ネットワークプログラムには別の制約もあるため，[ネットワークプログラミング](NETWORK-PROGRAMMING.md)も参照してください．

## 7．問題が起きたとき

| 症状 | 確認すること |
|---|---|
| Cコンパイルエラー | `#include`，関数宣言，PicoNIX独自ヘッダの使い方 |
| 未定義シンボル（`__aeabi_*`を含む） | 未対応の依存関係，コンパイラ補助関数やリンク設定 |
| PIC・サイズ検査エラー | 位置依存コード，未対応の再配置，イメージ領域やスタック予約との重複 |
| `/bin`にコマンドがない | CMakeへの追加，シンボル宣言，`register_image()`，最新UF2の書き込み |
| 実行時に起動できない | タスクスロットの空きとプログラム登録 |
| 端末の動作がおかしい | canonical・echo設定と復元処理．[TTY](../../../literacy/TTY.md)を参照 |

ホスト側の回帰テストは次のように実行します．

```sh
python3 tools/run_tests.py
python3 tools/run_tests.py --all  # ARMツールチェーン依存テストも実行
```

ソースレベルのテストだけでは，ファームウェアのビルドや実機動作までは保証できません．実行中の調査には`ps`，`cat /proc/tasks`，`trace on`，`trace off`，`trace show`などを使います．[トレース](../../../literacy/TRACE.md)ではシステムコールとコンテキストスイッチ，[メモリ参照](../../../literacy/MEM.md)ではRAM・Flashの読み取り専用観察を説明しています．

## 8．リアルタイムキー入力（noncanonical + O_NONBLOCK）

`read()`で改行を待たずにキーを読み取り，入力がない間も処理を続ける例です．ゲームや対話型ツールでは，**TTYの入力方式**と**FDの待機方式**を別々に設定します．

- `ioctl(0, OS_TTY_SET_CANON, 0)`：行単位入力を解除し，文字単位で受け取ります．
- `ioctl(0, OS_TTY_SET_ECHO, 0)`：入力文字の自動表示を止めます．
- `fcntl(0, F_SETFL, flags | O_NONBLOCK)`：入力がなくても`read()`を待機させません．

以下のコードは`userland/apps/keypoll.c`などに保存できる例です．PicoNIX独自のTTY用`ioctl`定数を使用し，POSIXの`termios`は必要ありません．

```c
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"

int main(void)
{
    int canon = ioctl(0, OS_TTY_GET_CANON, 0);
    int echo  = ioctl(0, OS_TTY_GET_ECHO, 0);
    int flags = fcntl(0, F_GETFL, 0);
    int result = 1;

    if (canon < 0 || echo < 0 || flags < 0) {
        puts("A TTY is required.");
        return 1;
    }

    if (ioctl(0, OS_TTY_SET_CANON, 0) < 0 ||
        ioctl(0, OS_TTY_SET_ECHO, 0) < 0 ||
        fcntl(0, F_SETFL, flags | O_NONBLOCK) < 0) {
        puts("Cannot configure TTY.");
        goto restore;
    }

    puts("Counter runs without input. Press q to quit.");
    for (unsigned count = 0;; ++count) {
        char key;
        int n = read(0, &key, 1);
        if (n == 1 && (key == 'q' || key == 'Q')) {
            result = 0;
            break;
        }
        if (n < 0) { /* Actual input error. */
            puts("TTY read error.");
            break;
        }
        /* n == 0: no key available in nonblocking mode. */
        if (count % 10 == 0)
            printf("Count: %u\n", count);
        msleep(100);
    }

restore:
    fcntl(0, F_SETFL, flags);
    ioctl(0, OS_TTY_SET_ECHO, echo);
    ioctl(0, OS_TTY_SET_CANON, canon);
    return result;
}
```

**PicoNIXでの重要な挙動：** TTYの非ブロッキング`read()`は，入力可能な文字がなければ**0**を返します（Linuxの`EAGAIN`を前提にしません）．noncanonical入力ではCtrl+D（0x04）は通常の入力バイトとして読み取られ，EOFにはなりません．canonical入力でのCtrl+Dとは動作が異なります．負の戻り値はエラーとして扱います．`msleep(100)`を挟むことで，入力のない間の無駄な繰り返しを抑えます．

開始時にcanonical・echo・FDフラグを保存し，終了時と設定途中の失敗時に戻します．**Ctrl+Cや`kill`などでアプリが強制終了した場合，このCコードの復元処理は実行されません**．前景タスクのTTY設定はカーネル側の復元機構に依存します．FDフラグはタスク固有なので，通常終了時に明示的に戻します．

この例は既存の`greet`と同様にCMakeへの追加とカーネル登録が必要です（上記の手順を`keypoll`に読み替えてください）．`stty raw`を事前に実行する必要はありません．TTYの規則は[TTY](../../../literacy/TTY.md)，実際のゲームでの利用は`userland/apps/mb.c`と`userland/apps/pong.c`を参照してください．

## 9．関連文書

- [Cライブラリ](LIBC.md)：アプリで使用できる関数と制限
- [システムコール](../../../reference/SYSCALLS.md)：番号，引数，戻り値，ABI
- [アセンブリプログラミング](../asm/ASM.md)・[Python](../python/PYTHON.md)：PicoNIX上で作るプログラム（ホストでCをビルドする方式とは異なる）
- [全体構造](../../../reference/ARCHITECTURE.md)・[内部構造](../../../reference/INTERNAL.md)：PICロード，スケジューラ，カーネル実装
- [ネットワークプログラミング](NETWORK-PROGRAMMING.md)：Pico W上のTCP/UDPアプリ開発
