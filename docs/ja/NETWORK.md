# Pico Wネットワーク接続

[English](../en/NETWORK.md) | [セットアップ](SETUP.md)

Pico W版のPicoNIXは標準でWi-Fiアクセスポイント（AP）として動作し，DHCPで接続端末へIPアドレスを割り当てます．ビルド時にSTAモードを選択すると，既存のWi-Fiネットワークにも参加できます．USB CDCコンソールとは別に，ネットワーク経由のシェルを利用できます．通常のPicoにはWi-Fi機能はありません．なお，現在のPico W向けファームウェアは通常のPicoでも動作確認済みですが，Wi-Fi機能は利用できません．

STAモードのビルド設定と接続状態は[Wi-Fiモード](WIFI-MODES.md)を参照してください．

## ビルド

Pico SDK 2.1.1とツールチェーンを用意し，リポジトリのルートで実行します．

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

別途インストールしたツールチェーンを使う場合は`PICO_TOOLCHAIN_PATH`も指定してください．生成物は`build-pico_w/piconix.uf2`です．BOOTSELモードで書き込んでください．`PICONIX_WIFI_LAYOUT`オプションは廃止されています．通常のPico向けビルドは`-DPICO_BOARD=pico`を指定します．

## Wi-Fi接続

- SSID：`PicoNIX-XXXXXXXX`（末尾は個体ごとに変わります．起動表示を確認してください）
- パスワード：`piconix1234`
- AP側アドレス：`192.168.4.1`
- DHCP割り当てアドレス：`192.168.4.2`（単一クライアント向け）

Wi-Fiに接続すると，通常はDHCPでIPアドレスが自動設定されます．インターネット接続を提供するアクセスポイントではありません．

## リモートシェルへの接続

| 方式 | ポート | コマンド |
|---|---:|---|
| Telnet | TCP 23 | `telnet 192.168.4.1` |

ネットワークTTYは`/dev/wifi`です．Telnetと生TCPは同じシェルを共用するため，ネットワーク接続は同時に1つだけ利用できます．USB CDCコンソールは独立しています．

### 接続の終了

**Telnet**：`Ctrl + ]`で`telnet>`プロンプトへ移動し，`quit`とEnterを入力します．

**socat**：`escape=0x1d`を指定して起動した場合は，`Ctrl + ]`で終了します．`raw`モードでは通常の`Ctrl + C`は端末へ送られます．

Linuxでは必要に応じて`sudo apt install telnet socat`でクライアントを導入してください．Windows 10/11では「Windowsの機能」からTelnetクライアントを有効にするか，管理者権限で`dism /online /Enable-Feature /FeatureName:TelnetClient`を実行します．Windows標準環境では`socat`は通常利用できません．

## 注意事項

Telnetと生TCPのシェル通信は暗号化されません．信頼できる実験用ネットワークでのみ使用してください．APのパスワードは公開された固定値なので，機密情報を送信しないでください．端末の操作性はTelnetクライアントの実装によって異なる場合があります．

開発・保守向けのDHCP障害調査記録は[NETWORK-DHCP-NOTES.md](../history/ja/NETWORK-DHCP-NOTES.md)にまとめています．

## TCP/UDPアプリケーションとリモートデバイス制御

Pico WではTelnet（TCP 23）に加え，ユーザプロセスの`nc`，`httpd`（TCP 80），`tcp_echod`，`udp_echod`が利用できます．UDP 2000では許可されたデバイスを遠隔操作できます．APIとテスト方法は[ソケットAPIとネットワークアプリケーション](NETWORK-PROGRAMMING.md)を参照してください．

UDP 2000番の通信仕様と操作例は[リモートデバイス制御](REMOTE-DEVICE.md)を参照してください．
