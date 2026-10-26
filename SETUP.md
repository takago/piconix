# PicoNIX セットアップガイド

この文書は**Ubuntu系Linux + Raspberry Pi Pico（RP2040）+ Pico SDK 2.1.1**を想定しています．プロジェクトのビルドスクリプトは`tools/build_sdk.sh`です．

## 1．必要なもの

- Raspberry Pi Pico（通常モデル，RP2040）とUSBデータケーブル
- Pico SDK 2.1.1（サブモジュールを含む）
- ARM用GCCツールチェーン（例：`~/.pico-sdk/toolchain/14_2_Rel1`）
- CMake，Make，Python 3，Gitなど
- UF2を書き込むためのPC環境と，USB CDCに接続できるシリアル端末

SDKとツールチェーンは事前に導入されているものとします．導入方法は使用OSやSDKの配布形態により異なります．

## 2．ソースの取得

```bash
git clone <公開後のPicoNIXリポジトリURL>
cd piconix
```

URLはGitHub公開後の実際のアドレスに置き換えてください．ZIPから展開した場合は，展開先ディレクトリに移動します．

## 3．ビルド

最初に実行権限を付けます．Git上で実行ビットが保存されていれば省略できます．

```bash
chmod +x ./tools/build_sdk.sh
```

以下は，ご利用の環境に合わせたコマンド例です．

```bash
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh
```

`~`はシェルの展開規則に依存するため，環境変数の値には`$HOME`を使うと確実です．`PICO_SDK_PATH`はSDKの実際のインストール先に合わせて変更してください．

スクリプトは，ホスト側の検査を実行してから，`cmake -S . -B build -DPICO_BOARD=pico -DPICONIX_KERNEL_RAM=ON`で構成し，ビルドします．`MinSizeRel`（通常は `-Os`）がデフォルトのビルド構成です．明示的に `-DCMAKE_BUILD_TYPE=Release` などを指定した場合は，その指定を優先します．既存の `build/` にビルド構成が保存されている場合は `-DCMAKE_BUILD_TYPE=MinSizeRel` を明示するか，新しいビルドディレクトリを使用してください．

`build/`を毎回削除する処理は含まれていません．

## 4．生成物

通常，次のファイルが生成されます．

```text
build/piconix.uf2
build/piconix.elf
build/piconix.map  （生成環境による）
```

OSの表示名はPicoNIXですが，**現段階のCMakeターゲット名とUF2名は`piconix`**です．

## 5．Picoへの書き込み

1. Picoの**BOOTSEL**ボタンを押しながらUSB接続し，USBストレージ（`RPI-RP2`）として認識させます．
2. `build/piconix.uf2`をそのドライブへコピーします．
3. コピー後にPicoが再起動します．
4. USB CDCシリアルポートへ端末ソフトで接続します．Linuxでは環境により`/dev/ttyACM0`などになります．

例えば，シリアル端末に`picocom`がインストールされている場合：

```bash
picocom -b 115200 /dev/ttyACM0
```

USB CDCでは115200という設定値は実際のUART転送速度を決めませんが，端末ソフトの設定として使用できます．デバイス名は環境によって異なります．

## 6．動作確認

```text
piconix# help
piconix# ps
piconix# omikuji 3
piconix# blink &
piconix# ps
piconix# kill 2
piconix# cat /sys/proc/uptime
```

`kill`のPIDは`ps`で確認した実際の値を指定してください．前景の`blink`はCtrl+Cで終了できます．`omikuji`だけを入力すると，秒数を標準入力から尋ねます．

## 7．トラブルシューティング

- **`PICO_SDK_PATH`エラー**：SDKの場所，バージョン，サブモジュールを確認してください．
- **`No installed picotool ... found`**：SDKが対応する`picotool`を見つけられず，ソースから取得・ビルドする場合の警告です．通常，それ自体はビルド失敗を意味しません．
- **USBコンソールに表示されない**：USB CDCの認識と端末の再接続を確認してください．起動直後のメッセージは接続前に失われる場合があります．
- **GPIO25のLEDが点滅しない**：通常のPicoか，対象GPIOが異なるボードかを確認してください．
- **RAMスロット検査で失敗**：ビルドログを保存し，SDK・ツールチェーン・リンカスクリプトを確認してください．
- **UART1の`cat`がすぐ終了する**：受信データが約200 ms途絶えると終了する設計です．

## 8．補足

UART0コンソールへの切り替えはCMakeオプション`-DPICONIX_CONSOLE_USB=OFF`で指定できますが，通常の利用はUSB CDCを推奨します．内部構造は[INTERNAL.md](INTERNAL.md)を参照してください．

### 起動バナーが表示されない場合

USB CDCはリセット後に再認識されます．端末が再接続された後，Shellがバナーを表示します．端末ソフトが自動再接続しない場合はポートを開き直してください．
