# PicoNIXのCライブラリとユーザアプリケーションAPI

[English](../../../../en/development/apps/c/LIBC.md) | [Cアプリケーション開発](PROGRAMMING.md) | [システムコール](../../../reference/SYSCALLS.md)

PicoNIXは，ユーザプログラム向けに**独自の小規模なCライブラリ**を提供します．ISO Cの標準ライブラリやPOSIX libcを完全に実装したものではありません．ホスト環境のライブラリ関数がそのまま使えるとは限りません．主な実装は，`userland/lib/libc.c`（文字列・数値変換・書式付き入出力），`userland/lib/syscalls.c`（システムコールのラッパー），`userland/lib/crt0.c`（起動と終了）に分かれています．

## ヘッダと利用できる関数

下表は，リポジトリ内のヘッダ宣言と実装に基づくものです．正確な関数宣言は各ヘッダを参照してください．

| ヘッダ | 関数・定義 | 機能 |
|---|---|---|
| `stdio.h` | `putchar`, `getchar`, `puts`, `fgets` | 文字・行の入出力 |
| `stdio.h` | `printf`, `sprintf`, `snprintf` | 簡易書式付き出力 |
| `stdio.h` | `FILE`, `stdin`, `stdout`, `stderr`, `EOF` | 最小限のストリーム定義．`FILE`はFDを保持 |
| `string.h` | `strlen`, `strcpy`, `strncpy`, `strcat`, `strcmp`, `strncmp`, `strchr`, `strstr` | 文字列操作 |
| `string.h` | `memcpy`, `memmove`, `memset`, `memcmp` | メモリ操作 |
| `stdlib.h` | `atoi`, `strtol`, `abs` | 整数変換・絶対値 |
| `unistd.h` | `read`, `write`, `open`, `close`, `unlink`, `lseek`, `getdents` | ファイル・FDの操作 |
| `unistd.h` | `pipe`, `dup`, `dup2` | パイプ・FD複製 |
| `unistd.h` | `getpid`, `kill`, `waitpid`, `spawn`, `spawn_tmp`, `tty_spawn`, `program_id`, `set_foreground`, `_exit` | タスクの起動・終了・待機 |
| `unistd.h` | `msleep`, `yield`, `flash_id`, `get_tty`, `console_ready`, `chdir`, `getcwd`, `ioctl` | OS・ディレクトリ・端末操作 |
| `unistd.h` | `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`, `sendto`, `recvfrom`, `tcp_status`, `shutdown` | 小規模なIPv4 TCP/UDPソケットAPI |
| `unistd.h` | `bind_udp`, `sendto_udp`, `recvfrom_udp`, `tcp_listen`, `tcp_accept`, `tcp_connect` | PicoNIX固有の低水準ネットワークAPI |
| `fcntl.h` | `fcntl`, `O_*`, `F_GETFL`, `F_SETFL` | FDフラグ，非ブロッキング設定 |
| `piconix_dirent.h` | `piconix_dirent_t` | 固定形式のディレクトリエントリ |
| `piconix.h` | `OS_*`, `piconix_udp_packet_t`, `piconix_tcp_endpoint_t` | OSの共通定数・構造体 |

`piconix.h`と`piconix_dirent.h`は`include/`に配置され，必要に応じて`unistd.h`などから取り込まれます．アプリケーション向けの専用ヘッダには`trace.h`や`walk.h`もありますが，標準Cライブラリの一部ではありません．

## よく使う関数の引数と戻り値

| 関数 | 引数 | 戻り値・動作 |
|---|---|---|
| `printf(fmt, ...)` | 書式文字列と値 | 出力文字数．書き込み失敗時は`-1` |
| `snprintf(dst, size, fmt, ...)` | 出力先，容量，書式文字列と値 | 本来出力する文字数．`size > 0`で有効な出力先ならNUL終端 |
| `puts(s)` | NUL終端文字列 | 文字列と改行を出力．失敗時は`EOF` |
| `putchar(c)` | 文字コード | 書き込んだバイト値または`EOF` |
| `getchar()` | なし | 入力した1バイト，読めなければ`EOF` |
| `fgets(s, size, stream)` | 出力先，容量，ストリーム | 成功時は`s`．1文字も読めなければNULLポインタ．簡易実装 |
| `strlen(s)` | NUL終端文字列 | 終端NULを含まないバイト数 |
| `strtol(s, endptr, base)` | 文字列，終端位置の格納先，基数 | `long`値．オーバーフロー時は`LONG_MIN`または`LONG_MAX`に制限（`errno`なし） |
| `read(fd, buf, count)` | FD，バッファ，最大バイト数 | 読み取り数，対象によりEOF時は0，エラー時は負のOSエラー値 |
| `write(fd, buf, count)` | FD，バッファ，バイト数 | 書き込んだバイト数または負のOSエラー値 |
| `open(path, flags)` | パス，PicoNIXの`O_*`フラグ | FDまたは負のOSエラー値 |
| `msleep(milliseconds)` | ミリ秒 | OSサービスの結果（`int`） |
| `socket(domain, type, protocol)` | アドレス族，種類，プロトコル | FDまたは負のOSエラー値 |

**全関数の正確な型・引数**はヘッダ，**SVCの番号・レジスタ規約・カーネル処理**は[システムコール](../../../reference/SYSCALLS.md)を参照してください．`connect()`の非同期動作，`tcp_status()`，`accept()`と`recvfrom()`の非ブロッキング動作は[ネットワークプログラミング](NETWORK-PROGRAMMING.md)に説明があります．

実践例：TTYの`ioctl()`とFDの`fcntl()`を組み合わせてキー入力をポーリングする場合は，[Cアプリケーション開発ガイドの第8章「リアルタイムキー入力」](PROGRAMMING.md)を参照してください．


`shutdown(fd, SHUT_WR)`はTCPの送信側だけを閉じます．汎用的なPOSIXの全`shutdown()`モードに対応するものではありません．

## 重要な制限

- 書式付き出力は`%s`，`%c`，`%d`，`%i`，`%u`，`%x`，`%X`，`%%`に対応します．整数の桁幅・精度・パディングと一部の長さ修飾子にも対応しますが，**浮動小数点の書式には対応しません**．詳細は`userland/lib/libc.c`を参照してください．
- 一般的なCライブラリの全関数が存在するわけではありません．例えば，このライブラリのヘッダには**`fopen`，`fclose`，`fprintf`，`scanf`，`malloc`，`free`，`realloc`，`exit`は宣言されていません**．
- `sprintf()`は出力先のサイズを指定できません．バッファ破壊を防ぐため，通常は`snprintf()`を使用してください．
- `fgets()`は`stream->fd`から直接読み取ります．一般的なバッファ付き標準入出力とは異なる簡易実装です．
- OS APIの多くは**負の`OS_*`エラー値を直接返し**，POSIXの`errno`を使用しません．
- `O_*`や`F_*`の数値はホストPOSIXと同一ではありません．`lseek()`は`/tmp`と`/dev/shm`で`SEEK_SET`，`SEEK_CUR`，`SEEK_END`をサポートし，`/dev/mem`では`SEEK_SET`のみです．
- ソケットはIPv4用の限定的なAPIです．TCPでは`bind()`で待ち受けが始まり，`listen()`は互換性のための空操作です．

## `fgets()`の境界条件

現行の`userland/lib/libc.c`では，`fgets(s, size, stream)`は`size <= 0`またはNULL引数ではNULLを返します．`size == 1`では1バイトの出力領域に終端NULだけを書いて`s`を返し，実際の入力は読み取りません．`size > 1`で最初の1文字を取得できなかった場合はNULLを返します．通常のファイルストリーム機能を想定せず，端末の入力条件とFDの状態を確認してください．

## プログラム例

```c
#include "stdio.h"
#include "unistd.h"

int main(int argc, char *argv[]) {
    printf("PID: %d\n", getpid());
    if (argc > 1)
        printf("Argument: %s\n", argv[1]);
    msleep(100);
    return 0;
}
```

ユーザプログラムはFlashに格納されるPICイメージとしてビルドされ，起動時にRAMスロットへコピーされます．`main()`からの復帰後の終了処理は`crt0.c`が担当します．登録・ビルド方法は[Cアプリケーション開発](PROGRAMMING.md)を参照してください．

## 実装との対応

- `userland/lib/stdio.h`，`string.h`，`stdlib.h`，`unistd.h`，`fcntl.h`：アプリケーション向けヘッダ
- `userland/lib/libc.c`：Cライブラリの基本関数
- `userland/lib/syscalls.c`：システムコールのラッパーとソケット互換関数
- `userland/lib/crt0.c`：エントリ処理・終了処理
- `include/piconix.h`，`include/piconix_dirent.h`：OS定数・共有構造体

### ディレクトリエントリのデバイスID

`ls -l /dev`などの長形式表示では，デバイスファイルにカーネル内部IDを表示します．`/dev/gpio`などの下位ディレクトリでも利用できます．`getdents()`の`piconix_dirent_t`には`device_id`（`uint16_t`）を追加し，IDを持たないエントリには`0xffff`を設定します．この変更はディレクトリエントリABIを変更するため，カーネルとユーザプログラムを同時に再ビルドしてください．

`getdents()`の`device_id`フィールドはディレクトリ項目のIDです．`/dev`と`/proc`ではデバイスID，`/bin`ではプログラムID，`/tmp`・`/etc`ではRAMファイルのスロット番号（0始まり）を返します．ディレクトリやIDのない項目は`0xffff`です．`ls -l`は`TYPE ID SIZE NAME`の見出しを表示し，`p`をプログラム，`f`をRAMファイル，`c`をデバイス，`d`をディレクトリとして区別します．スロット番号はファイルの削除・再生成で変わるため，恒久的な識別子ではありません．
