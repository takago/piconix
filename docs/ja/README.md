# PicoNIX

[English](../en/README.md) | [目的別の文書案内](DOCS.md)

PicoNIXは，**Raspberry Pi Pico（RP2040）向けの小型UNIX風OS**です．Linuxを移植したものではなく，タスク管理，システムコール，簡易ファイル，デバイスI/Oの仕組みを少ないコードで理解するための実験的なOSです．

アプリケーション領域は24 KiB×6スロットです．

## できること

- **Pico WのユーザプロセスでTCP/UDP通信**：`socket()`，`send()`，`recv()`，`sendto()`，`recvfrom()`などを提供し，`nc`，`httpd`，`tcp_echod`，`udp_echod`を実行できます．詳細は[ソケットAPIとネットワークアプリケーション](NETWORK-PROGRAMMING.md)を参照してください．

- SysTick／PendSVによるプリエンプティブなタスク切り替え
- SVCによるカーネルとRAMアプリケーションの呼び出し境界
- `ps`，`kill`，`sh`，`getty`，`blink`，`omikuji`，`python`，`hello`，`hexdump`などのコマンド
- `&`による簡易バックグラウンド実行，`Ctrl+C`による前景タスクの中断
- `/dev`と`/proc`の仮想ファイル，`/tmp`の揮発性ファイル
- 標準入出力の簡易リダイレクト（`<`，`>`，`>>`）と最大3コマンドのパイプライン（`|`）
- 整数演算やGPIOを扱える小型のPython風インタプリタ

`nc -u -N`は標準入力のEOFでUDP送信を終了でき，前景コマンドやパイプラインはCtrl+Cで中断できます．詳細は[ncの使い方](NC.md)を参照してください．

## まず動かす

Pico SDK 2.1.1とARM GCCを導入したLinux環境で，次のようにビルドします．パスは環境に合わせて変更してください．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

生成した`build-pico/piconix.uf2`をBOOTSELモードのPicoへコピーし，USB CDC端末に接続します．詳細は[セットアップ](SETUP.md)を参照してください．

```text
piconix# help
piconix# help gpio
piconix# ps
piconix# blink &
piconix# cat /proc/uptime
piconix# echo hello > /tmp/hello.txt
piconix# cat /tmp/hello.txt
piconix# python
```

## 2つの端末で使う

USBコンソールのShellから`getty /dev/uart1 sh &`を実行すると，UART1に別のShellを起動できます．UART1側で起動したプログラムには同じ端末の標準入出力が継承されます．Python対話環境なら`getty /dev/uart1 python &`です．UART1は115200 bps，8N1で接続し，端末側のローカルエコーを利用してください．同一端末で複数の対話プログラムを競合させないため，占有中は新しい`getty`の起動に失敗します．

## ハードウェアと接続

標準構成はUSB CDCコンソールと8×8 LEDマトリクスを想定しています．UART1はGP20（TX）／GP21（RX），PWMはGP18／19，ADCはGP26～28です．配線，電流制限，3.3 Vロジックの注意事項は[デバイスガイド](DEVICES.md)にまとめています．

## 設計の特徴と制限

RP2040のRAMのうち，カーネルに64 KiB，アプリケーション用に24 KiBスロット6個を割り当てます．アプリケーションはPICとしてFlashに埋め込まれ，起動時に空きスロットで動作します．同時実行は最大6タスクです．

PicoNIXは完全なUNIX互換OSではありません．汎用シグナル配送，シグナルハンドラ，プロセスごとのメモリ保護，ELF実行ファイルの動的読み込みは実装していません．`Ctrl+C`と`kill`は，UNIXのシグナル送信ではなく，カーネルによるタスクの直接終了です．`/tmp`の内容は再起動すると消えます．

## 次に読む文書

用途別の詳しい案内は[ドキュメント目次](DOCS.md)にあります．


| やりたいこと | 読む文書 |
|---|---|
| 必要な環境を準備し，書き込む | [セットアップ](SETUP.md) |
| RAM配置，タスクの仕組みを理解する | [内部構造](INTERNAL.md) |
| システムコールの番号と呼び出し規約を確認する | [システムコール一覧](SYSCALLS.md) |
| `as`でアセンブリ言語を試す | [簡易Thumbアセンブラ](ASM.md) |
| パイプ，`grep`，FDの仕組みを知る | [パイプと標準入出力](PIPE.md) |
| CでRAMアプリケーションを作る | [アプリ開発](PROGRAMMING.md) |
| TCP/UDPソケットとネットワークサーバを使う | [ソケットAPIとネットワークアプリケーション](NETWORK-PROGRAMMING.md) |
| GPIO・ADC・PWMなどのデバイスを使用する | [デバイスガイド](DEVICES.md) |
| UART1，`getty`，Ctrl+C／Ctrl+Dを理解する | [UARTと端末操作](UART.md) |
| `python`コマンドを使う | [Python風インタプリタ](PYTHON.md) |
| `stty`・`mb`・`reset`を使う | [端末設定と対話型入力](TTY.md) |
| 8×8 LEDマトリクスを使う | [デバイスガイド（フレームバッファ）](DEVICES.md) |
| カーネルトレースを見る | [トレース](TRACE.md) |


## ライセンス

GPL-3.0-or-later．詳細は`LICENSE`を参照してください．
