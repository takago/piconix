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
