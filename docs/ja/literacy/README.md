# PicoNIX

[English](../../en/literacy/README.md) | [目的別の文書案内](../../DOCS.ja.md)

**初めて学ぶ方へ：** [学習ガイド](LEARNING-PATH.md)では，準備からファイル，プロセス，プログラミングまで順番に試せます．

**ファイルブラウザ：** [fd](FD.md)はディレクトリ移動，定期更新表示，`/tmp`の編集に対応します．

**テーマ別実習：** [10テーマの学習ガイド](LEARNING-PATH.md)から，基本操作，標準I/O，デバイス，プロセス，メモリ，端末，モニタリング，ネットワークへ進めます．

PicoNIXは，**Raspberry Pi Pico（RP2040）向けの小型UNIX風OS**です．Linuxを移植したものではなく，タスク管理，システムコール，簡易ファイル，デバイスI/Oの仕組みを少ないコードで理解するための実験的なOSです．

アプリケーション領域は24 KiB×6スロットです．

## できること

- **Pico WのユーザプロセスでTCP/UDP通信**：`socket()`，`send()`，`recv()`，`sendto()`，`recvfrom()`などを提供し，`nc`，`httpd`，`tcp_echod`，`udp_echod`を実行できます．詳細は[ソケットAPIとネットワークアプリケーション](../development/apps/c/NETWORK-PROGRAMMING.md)を参照してください．
- **パケット観察と到達確認**：Pico Wでは`tcpdump`と`ping`を利用できます．[ネットワーク実習](NETWORKING.md)を参照してください．

- SysTick／PendSVによるプリエンプティブなタスク切り替え
- SVCによるカーネルとRAMアプリケーションの呼び出し境界
- `ps`，`kill`，`sh`，`getty`，`blink`，`omikuji`，`python`，`hello`，`hexdump`，`dd`，`vi`，`edit`などのコマンド
- `&`による簡易バックグラウンド実行，`Ctrl+C`による前景タスクの中断
- `/dev`と`/proc`の仮想ファイル，`/tmp`と`/etc`の揮発性ファイル
- 標準入出力の簡易リダイレクト（`<`，`>`，`>>`）と最大3コマンドのパイプライン（`|`）
- `dd`によるバイナリデータ転送と，2 KiBまでのRAMファイルを扱う`vi`・`edit`
- 整数演算やGPIOを扱える小型のPython風インタプリタ

`nc -u -N`は標準入力のEOFでUDP送信を終了でき，前景コマンドやパイプラインはCtrl+Cで中断できます．詳細は[ncの使い方](../reference/devices/NC.md)を参照してください．

## まず動かす

Pico SDK 2.1.1とARM GCCを導入したLinux環境で，次のようにビルドします．パスは環境に合わせて変更してください．

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

Pico Wでは`-DPICO_BOARD=pico`を`-DPICO_BOARD=pico_w`に変更します．生成した`build-pico/piconix.uf2`（Pico）または`build-pico_w/piconix.uf2`（Pico W）をBOOTSELモードのPicoへコピーし，USB CDC端末に接続します．詳細は[セットアップ](../setup/SETUP.md)を参照してください．

```text
piconix# help
piconix# help gpio
piconix# ps
piconix# blink &
piconix# cat /proc/uptime
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# cat -n /tmp/hello.txt
piconix# python
```

## 2つの端末で使う

USBコンソールのShellから`getty /dev/ttyS1 sh &`を実行すると，UART1に別のShellを起動できます．UART1側で起動したプログラムには同じ端末の標準入出力が継承されます．Python対話環境なら`getty /dev/ttyS1 python &`です．UART1は115200 bps，8N1で接続し，端末側のローカルエコーを利用してください．同一端末で複数の対話プログラムを競合させないため，占有中は新しい`getty`の起動に失敗します．

## ハードウェアと接続

標準構成はUSB CDCコンソールと8×8 LEDマトリクスを想定しています．UART1はGP20（TX）／GP21（RX），PWMはGP18／19，I²C1はGP26（SDA）／GP27（SCL），ADC2はGP28です．配線，電流制限，3.3 Vロジックの注意事項は[デバイスガイド](../reference/devices/DEVICES.md)にまとめています．

## 設計の特徴と制限

RP2040のRAMのうち，カーネルに56 KiB，アプリケーション用に24 KiBスロット6個を割り当てます．アプリケーションはPICとしてFlashに埋め込まれ，起動時に空きスロットで動作します．同時実行は最大6タスクです．

`cat -n`は空行を含む各行の先頭に番号を表示しますが，ファイルの内容は変更しません．

PicoNIXは完全なUNIX互換OSではありません．汎用シグナル配送，シグナルハンドラ，プロセスごとのメモリ保護，ELF実行ファイルの動的読み込みは実装していません．`Ctrl+C`と`kill`は，UNIXのシグナル送信ではなく，カーネルによるタスクの直接終了です．`/tmp`と`/etc`は同じRAMファイル領域（最大11ファイル，各最大2 KiB）を共有し，再起動すると内容は消えます．

## 次に読む文書

用途別の詳しい案内は[ドキュメント目次](../../DOCS.ja.md)にあります．


| やりたいこと | 読む文書 |
|---|---|
| PicoNIXの開発経緯と設計判断を知る | [開発日誌](../../DEVELOPMENT_DIARY.ja.md) |
| 必要な環境を準備し，書き込む | [セットアップ](../setup/SETUP.md) |
| OSの構造とソースコードの担当を把握する | [全体アーキテクチャ](../reference/ARCHITECTURE.md) |
| RAM配置，タスクの仕組みを理解する | [内部構造](../reference/INTERNAL.md) |
| システムコールの番号と呼び出し規約を確認する | [システムコール一覧](../reference/SYSCALLS.md) |
| `as`でアセンブリ言語を試す | [簡易Thumbアセンブラ](../development/apps/asm/ASM.md) |
| パイプ，`grep`，FDの仕組みを知る | [パイプと標準入出力](PIPE.md) |
| CでRAMアプリケーションを作る | [アプリ開発](../development/apps/c/PROGRAMMING.md) |
| TCP/UDPソケットとネットワークサーバを使う | [ソケットAPIとネットワークアプリケーション](../development/apps/c/NETWORK-PROGRAMMING.md) |
| GPIO・ADC・PWMなどのデバイスを使用する | [デバイスガイド](../reference/devices/DEVICES.md) |
| UART1，`getty`，Ctrl+C／Ctrl+Dを理解する | [UARTと端末操作](../reference/devices/UART.md) |
| `dd`によるファイル・RAM・nc転送を試す | [ddガイド](DD.md) |
| `vi`でテキストを編集する | [viエディタ](VI.md) |
| `python`コマンドを使う | [Python風インタプリタ](../development/apps/python/PYTHON.md) |
| I²C機器を接続し，`i2cctl`や`bh1750`を使う | [I²C1ガイド](../reference/devices/I2C.md) |
| `stty`と`fcntl()`の違いを理解する | [端末設定と対話型入力](TTY.md) |
| ANSI画面制御・全画面アプリの復旧を調べる | [端末の使い方と画面制御](TERMINAL.md) |
| 8×8 LEDマトリクスを使う | [デバイスガイド（フレームバッファ）](../reference/devices/DEVICES.md) |
| カーネルトレースを見る | [トレース](TRACE.md) |


## ライセンス

GPL-3.0-or-later．詳細は`LICENSE`を参照してください．

[文書一覧](../../DOCS.ja.md) | [English](../../en/literacy/README.md)
