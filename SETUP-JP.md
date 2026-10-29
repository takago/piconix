# セットアップ

[English](SETUP-EN.md) | [概要](README-JP.md)

Ubuntu／Debian系Linuxを想定した導入手順です．Raspberry Pi Pico（RP2040）とデータ通信可能なUSBケーブルを用意してください．

## 1．開発ツールのインストール

```sh
sudo apt update
sudo apt install git cmake ninja-build python3 unzip picocom \
    gcc-arm-none-eabi libnewlib-arm-none-eabi
sudo apt install clang lld llvm
```

ARM GCCはRP2040向けのビルドに使用します．Clang，LLD，LLVMは主にRAMアプリケーションのPIC検証で使用します．

## 2．Pico SDK 2.1.1の取得

```sh
mkdir -p ~/.pico-sdk/sdk
cd ~/.pico-sdk/sdk

git clone --branch 2.1.1 \
    https://github.com/raspberrypi/pico-sdk.git 2.1.1

cd 2.1.1
git submodule update --init --recursive
```

既にディレクトリがある場合は再度cloneせず，そのチェックアウトでサブモジュールを更新してください．

## 3．PicoNIXの展開

PicoNIXのZIPをダウンロードしたディレクトリで実行します．以下のファイル名は今回の配布物の例です．

```sh
unzip PicoNIX-<version>.zip
cd <extracted-directory>
chmod +x ./tools/build_sdk.sh
```

Gitから取得した場合は，PicoNIXリポジトリのルートディレクトリへ移動してください．

## 4．ビルド（MinSizeRel）

UbuntuのパッケージでインストールしたARM GCCを使う場合は，次のコマンドでビルドします．

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

**ARM GCC 14.2を別途`~/.pico-sdk/toolchain/14_2_Rel1`に導入済みの場合**は，次のようにツールチェーンを明示できます．このディレクトリはSDKの`git clone`や上記の`apt install`では自動的に作成されません．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DCMAKE_BUILD_TYPE=MinSizeRel
```

`MinSizeRel`（通常`-Os`）を明示してください．ビルドスクリプトはホスト側の監査とPIC検証も実行します．別のツールチェーンに切り替えた場合は，CMakeのキャッシュを避けるため`build/`を削除してから再ビルドしてください．

ビルド成果物は通常，`build/piconix.uf2`と`build/piconix.elf`です．

## 5．Picoへの書き込み

1. BOOTSELボタンを押しながらPicoをUSB接続します．
2. 現れた`RPI-RP2`ドライブに`build/piconix.uf2`をコピーします．
3. Picoが再起動したら，Linux側のUSB CDCポートを確認します．

```sh
ls -l /dev/ttyACM*
picocom -b 115200 /dev/ttyACM0
```

`/dev/ttyACM0`は一例です．実際のデバイス名は環境に合わせて変更してください．USB CDCでは`115200`は端末側の設定値であり，物理UARTの通信速度ではありません．

## 6．動作確認

```text
piconix# help
piconix# ps
piconix# tty
piconix# cat /proc/uptime
piconix# blink &
piconix# ps
```

前景で起動したプログラムはCtrl+Cで中断できます．PIDを指定する場合は`ps`で確認した番号を使用してください．

## 7．トラブルシューティング

| 症状 | 確認事項 |
|---|---|
| SDKが見つからない | `PICO_SDK_PATH`とサブモジュール |
| ARM GCCが見つからない | `arm-none-eabi-gcc --version`，`PICO_TOOLCHAIN_PATH`の指定先 |
| Clang／LLDが見つからない | `clang --version`，`ld.lld --version` |
| RAM領域の超過・リンクエラー | `MinSizeRel`，ビルドキャッシュ，32 KiBのアプリ領域 |
| `/dev/ttyACM0`が出ない | `sudo dmesg -w`でUSB列挙を確認，ケーブル・書き込みを確認 |
| シリアルポートを開けない | デバイス名，アクセス権，他の端末による占有 |
| GPIO25が動かない | Raspberry Pi Picoのボード種別 |

USBと別にUART1からShellを利用するには`getty /dev/uart1 sh &`を実行します．UART1の設定とCtrl+C／Ctrl+Dは[UARTと端末操作](UART-JP.md)を参照してください．内部のRAM配置は[内部構造](INTERNAL-JP.md)を参照してください．
