# セットアップ

[English](SETUP-EN.md) | [概要へ戻る](README-JP.md)

## 1．準備

- Raspberry Pi Pico（RP2040）とデータ通信可能なUSBケーブル
- Linux，Pico SDK 2.1.1，ARM GCC，CMake，Python 3
- UF2書き込み用のUSB接続とシリアル端末（例：`picocom`）

SDKとツールチェーンは事前に導入してください．以下は既存のインストール先を使う例です．

## 2．ビルド

PicoNIXのルートディレクトリで実行します．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

このスクリプトはホスト側検査を実行してから，`PICO_BOARD=pico`，`PICONIX_KERNEL_RAM=ON`で構成・ビルドします．`MinSizeRel`（通常`-Os`）が標準です．既存の`build/`に別の設定が残っている場合に備えて，コマンドでは明示指定しています．

成果物は通常`build/piconix.uf2`と`build/piconix.elf`です．ビルドスクリプトは`build/`を自動削除しません．

## 3．Picoへ書き込む

1. BOOTSELを押しながらPicoをUSB接続します．
2. 認識された`RPI-RP2`ドライブに`build/piconix.uf2`をコピーします．
3. 再起動後，USB CDCのシリアルポートを開きます．Linuxでは`/dev/ttyACM0`などです．

```sh
picocom -b 115200 /dev/ttyACM0
```

USB CDCの`115200`は端末側の指定であり，物理UARTのビットレートではありません．ポート名は環境によって異なります．

## 4．動作確認

```text
piconix# help
piconix# ps
piconix# cat /sys/proc/uptime
piconix# blink &
piconix# ps
piconix# kill 2
```

`kill`のPIDは固定ではありません．必ず`ps`で表示された値を使用してください．前景の`blink`は`Ctrl+C`で終了できます．

## 5．問題が起きた場合

| 症状 | 確認すること |
|---|---|
| SDKが見つからない | `PICO_SDK_PATH`とSDKのサブモジュール |
| ARMツールが見つからない | `PICO_TOOLCHAIN_PATH`と実際のGCCの場所 |
| RAMサイズ／リンクエラー | 必ず`MinSizeRel`でビルドしたか，領域を超えていないか |
| USB端末に何も出ない | Picoの再認識後に端末を開き直したか |
| GPIO25のLEDが動かない | ボードが通常のRaspberry Pi Picoか |

UART0をコンソールとして使う設定は`-DPICONIX_CONSOLE_USB=OFF`です．通常はUSB CDCを推奨します．内部のメモリ配置は[内部構造](INTERNAL-JP.md)を参照してください．
