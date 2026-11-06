# セットアップ

Ubuntu／Debian系Linuxを想定した導入手順です．Raspberry Pi PicoまたはPico W（RP2040）とデータ通信可能なUSBケーブルを用意してください．

## ハードウェアの準備

PicoNIXの起動とUSB CDCコンソールの操作だけなら，**Pico／Pico Wとデータ通信可能なUSBケーブル**で十分です．LEDマトリクスなどの外付け回路は必須ではありません．

GPIOや周辺機器を試す場合は，使用する部品と接続方法を事前に確認してください．

- [ハードウェア概要](../reference/hardware/HARDWARE.md)：対応ボードと安全上の前提
- [ピン割当と40ピン配置図](../reference/hardware/PINS.md)：GPIO番号と物理端子番号
- [配線ガイド](../reference/hardware/WIRING.md)：LED，スイッチ，PWM，I²C，UART，ADCの接続
- [デバイスガイド](../reference/devices/DEVICES.md)：接続後の操作，GPIO・ADC・PWM・フレームバッファ

**配線前の注意：** GPIOは3.3 V系です．LEDには電流制限抵抗と適切な駆動回路が必要です．USBケーブルを抜いた状態で配線し，ピンの用途と極性を確認してから通電してください．最新の割り当ては`kernel/pins.h`を基準にしてください．

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

PicoNIXのZIPをダウンロードしたディレクトリで実行します．次の`<version>`と`<extracted-directory>`は説明用の仮名です．実際のZIP名と展開先ディレクトリ名に置き換えて実行してください．

```sh
unzip "PicoNIX-<version>.zip"
cd "<extracted-directory>"
chmod +x ./tools/build_sdk.sh
```

Gitから取得した場合は，PicoNIXリポジトリのルートディレクトリへ移動してください．

## 4．ビルド（MinSizeRel）

使用するボードに応じて，`-DPICO_BOARD=pico`または`-DPICO_BOARD=pico_w`を指定します．以下は通常のPico向けの例です．

UbuntuのパッケージでインストールしたARM GCCを使う場合は，次のコマンドでビルドします．

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

**ARM GCC 14.2を別途`~/.pico-sdk/toolchain/14_2_Rel1`に導入済みの場合**は，次のようにツールチェーンを明示できます．このディレクトリはSDKの`git clone`や上記の`apt install`では自動的に作成されません．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

**Pico W向け**には，どちらのコマンドでも`-DPICO_BOARD=pico`を`-DPICO_BOARD=pico_w`に置き換えてください．

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

実機では，**Pico W向けにビルドしたファームウェアが通常のPicoでも起動・動作することを確認しています**．ただし通常のPicoにはWi-Fiチップがないため，Wi-Fi機能は利用できません．この互換動作は現在の実装で確認されたものであり，すべての将来の版で保証されるものではありません．ボードごとの正式なビルド指定は上記のとおりです．

`MinSizeRel`を指定してください．ファームウェアのカーネル部分は通常`-Os`で最適化されます．全ユーザアプリケーションはCMakeが選択したARMコンパイラで別途`-Oz`と`-fPIC`を付けてビルドします．Clang／LLDは独立したPIC回帰テストで使用します．ビルドスクリプトはホスト側の監査とPIC検証も実行します．別のツールチェーンに切り替えた場合は，CMakeのキャッシュを避けるため該当する`build-pico/`または`build-pico_w/`を削除してから再ビルドしてください．

ビルド成果物は，Pico向けでは`build-pico/piconix.uf2`と`build-pico/piconix.elf`，Pico W向けでは`build-pico_w/piconix.uf2`と`build-pico_w/piconix.elf`です．

### カーネル関数トレース（ビルド時の既定値はON）

通常ビルドには選抜したカーネル関数のトレース機能が組み込まれます（**ビルド時の既定値はON**）．ただし，**起動時のイベント記録はOFF**です．`trace run hello`で関数階層を，`trace run -s hello`でシステムコールのみを観察できます．ビルド時にONを明示する必要はありません．

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DPICONIX_KERNEL_FUNCTION_TRACE=ON
```

Pico Wの場合は`-DPICO_BOARD=pico_w`に変更します．ビルド・書き込み後は`trace run hello`などで呼び出し階層を確認できます．軽量な構成に切り替える場合は，`-DPICONIX_KERNEL_FUNCTION_TRACE=OFF`を明示して再ビルドしてください．**CMakeキャッシュに以前のON設定が残ることがあるため，オプションの省略だけではOFFに戻らない場合があります．**

ONでは選抜した51関数を計測します．対象外関数には`NO_TRACE`を指定してフックの負荷を抑えていますが，対象関数のFlash・スタック・実行時間への影響は残ります．詳細と制約は[トレースガイド](../guides/TRACE.md)を参照してください．

## 5．Pico／Pico Wへの書き込み

1. BOOTSELボタンを押しながらPicoまたはPico WをUSB接続します．
2. 現れた`RPI-RP2`ドライブに対象ボードに応じて`build-pico/piconix.uf2`または`build-pico_w/piconix.uf2`をコピーします．
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
| RAM領域の超過・リンクエラー | `MinSizeRel`，ビルドキャッシュ，24 KiBのアプリ領域 |
| `/dev/ttyACM0`が出ない | `sudo dmesg -w`でUSB列挙を確認，ケーブル・書き込みを確認 |
| シリアルポートを開けない | デバイス名，アクセス権，他の端末による占有 |
| GPIO25が動かない | Raspberry Pi Picoのボード種別 |

USBと別にUART1からShellを利用するには`getty /dev/ttyS1 sh &`を実行します．UART1の設定とCtrl+C／Ctrl+Dは[UARTと端末操作](../reference/devices/UART.md)を参照してください．内部のRAM配置は[内部構造](../reference/INTERNAL.md)を参照してください．

## Pico Wのネットワーク接続

Pico Wのビルド，Wi-Fi設定，Telnet／socatの接続と終了方法は[Pico WのWi-Fi・Telnet・生TCP接続](../reference/network/NETWORK.md)を参照してください．


## 次に学ぶこと

接続できたら，[学習ロードマップ](../ROADMAP.md)に沿って，ファイル，パイプ，タスクを順に試してください．PCで実行するビルドコマンドと，`piconix#`で実行するOSコマンドを混同しないようにします．

---

**関連リンク：** [English](../../en/setup/SETUP.md) ｜ [操作ガイド一覧](../guides/README.md)
