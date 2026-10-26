# PicoNIX

[English](README-EN.md) | [文書一覧](#文書一覧)

PicoNIXは，**Raspberry Pi Pico（RP2040）向けの小型UNIX風OS**です．Linuxを移植したものではなく，タスク管理，システムコール，簡易ファイル，デバイスI/Oの仕組みを少ないコードで理解するための実験的なOSです．

## できること

- SysTick／PendSVによるプリエンプティブなタスク切り替え
- SVCによるカーネルとRAMアプリケーションの呼び出し境界
- `ps`，`kill`，`blink`，`omikuji`，`python`などのコマンド
- `&`による簡易バックグラウンド実行，`Ctrl+C`による前景タスクの中断
- `/dev`と`/sys/proc`の仮想ファイル，`/tmp`の揮発性ファイル
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
piconix# cat /sys/proc/uptime
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# python
```

## 設計の特徴と制限

RP2040のRAMを64 KiB単位で利用します．カーネル領域のほか，Shell，Blink，Omikuji／Python用の固定スロットがあります．アプリケーションはあらかじめFlashに埋め込まれ，起動時に所定のRAM領域で動作します．**OmikujiとPythonは同じスロットを使うため同時実行できません．**

PicoNIXは完全なUNIX互換OSではありません．汎用シグナル配送，シグナルハンドラ，パイプ，プロセスごとのメモリ保護，ELF実行ファイルの動的読み込みは実装していません．`Ctrl+C`と`kill`は，UNIXのシグナル送信ではなく，カーネルによるタスクの直接終了です．`/tmp`の内容は再起動すると消えます．

## 文書一覧

| やりたいこと | 読む文書 |
|---|---|
| 必要な環境を準備し，書き込む | [セットアップ](SETUP-JP.md) |
| RAM配置，タスク，システムコールを理解する | [内部構造](INTERNAL-JP.md) |
| CでRAMアプリケーションを作る | [アプリ開発](PROGRAMMING-JP.md) |
| `python`コマンドを使う | [Python風インタプリタ](PYTHON-JP.md) |

## ライセンス

GPL-3.0-or-later．詳細は`LICENSE`を参照してください．
