# ネットワーク実習（Pico W）

[English](../../en/literacy/NETWORKING.md) | [学習ガイド](LEARNING-PATH.md) | [ネットワーク仕様](../reference/devices/NETWORK.md) | [モニタリング実習](MONITORING.md)

この実習では，**Wi-Fiへの接続，IPアドレス，リモートシェル，TCP通信とその観察**を順に学びます．**Pico Wが必要**です．通常のPicoとUSB接続だけではネットワーク通信の実習はできません．最初は既定のAPモードで，インターネットから切り離した実験用ネットワークを使います．

**安全上の注意：** PicoNIXのTelnetとHTTPは暗号化されず，HTTPは認証もありません．大学や公共の無線LAN，インターネットへ公開しないでください．APの既定パスワードは公開値なので，機密情報は送信しないでください．GPIOやPWMを動かすHTTP APIもあるため，安全な機器以外は接続しないでください．

## 1．Pico Wを準備する

[セットアップ](../setup/SETUP.md)に従い，Linux PCでPico W用ファームウェアをビルドして書き込みます．以下のコードブロックはPicoNIX側で実行します．APモードでのSSIDは個体ごとに異なります．実際のSSIDを起動表示で確認し，PCまたはスマートフォンをそのAPに接続します．

```text
piconix# net
piconix# cat /proc/net/wifi
```

**確かめよう：** `MODE`，`STATUS`，`IP`などを読み取り，APモードとSTAモードの違いを説明してください．項目は状態により変わります．通常のAP構成ではPico Wは`192.168.4.1`，接続PCはDHCPで`192.168.4.2`を受け取ります．複数クライアントへDHCPを提供する構成ではありません．詳しくは[Wi-Fiモード](../reference/devices/WIFI-MODES.md)を参照してください．

## 2．Linux PCからリモートシェルを使う

APに接続済みの**Linux PC側の端末**から実行します．`telnet`コマンドがない場合はPC側で導入してください．

```sh
telnet 192.168.4.1
```

Telnet側で`help`，`ps`を実行し，USB側シェルの表示と比較してください．終了はPCのTelnetクライアントで`Ctrl+]`，`quit`です．**確かめよう：** 通信路が変わっても同じOSのプロセス情報を参照できる理由を考えましょう．[接続とTTYの仕様](../reference/devices/NETWORK.md)も参照してください．

## 3．HTTPサーバとTCPを試す

Pico W版では`httpd`が起動時に自動起動する構成です．PCのブラウザから`http://192.168.4.1/`へアクセスします．または，**Linux PC側**で次を実行します．

```sh
curl http://192.168.4.1/
```

`curl`が使えるのはPC側で，PicoNIXには`curl`コマンドはありません．起動状態を確かめたい場合はPicoNIXで`ps`を実行します．HTTPはTCP 80，TelnetはTCP 23です．**確かめよう：** IPアドレスとポート番号がそれぞれ何を識別するのか説明してください．詳しくは[HTTPサーバ](../reference/devices/HTTPD.md)と[`nc`の仕様](../reference/devices/NC.md)を参照してください．

## 4．通信中のシステムを観察する

USB側のPicoNIXシェルで次を実行します．

```text
piconix# ps
piconix# lsof
piconix# net -r
piconix# cat /proc/net/memory
```

**確かめよう：** `httpd`がユーザプロセスであること，`lsof`が開いたFDを表示すること，ARPがIPアドレスとMACアドレスを対応付けることを確認してください．ARP表は通信の有無によって変化し，空でも異常とは限りません．ネットワークのメモリ使用量やポーリング状況は，[ネットワーク仕様](../reference/devices/NETWORK.md)に詳しく記されています．

## 5．発展：TCPクライアントとループバック

Pico Wのネットワークスタックでは，`127.0.0.1`を同じPicoNIX自身のアドレスとして使用できます．`httpd`が稼働している場合は次を試せます（PicoNIX側）．

```text
piconix# nc 127.0.0.1 80
```

接続後，HTTPリクエスト`GET / HTTP/1.0`と空行を送ると，サーバから応答が返ることがあります．端末クライアントで改行の送られ方が異なるため，ここは**発展課題**とします．`nc`はPicoNIX上にありますが，`ping`は実装されていません．

## 振り返り

- APとSTAでは，Pico Wはどちらの役割を担うでしょうか．
- TelnetとHTTPはどちらもTCPを使うのに，ポート番号が異なるのはなぜでしょうか．
- `net`，`lsof`，`/proc/net/memory`は通信のどの側面を示すでしょうか．
- PCで通信を試した時刻とPicoNIXの観察結果を記録し，変化の有無を考察してください．

プログラムからTCP/UDPを使う場合は[Cでのネットワークプログラミング](../development/apps/c/NETWORK-PROGRAMMING.md)へ進みます．デバイスの遠隔制御は認証のない別サービスなので，利用前に[安全上の注意と仕様](../reference/devices/REMOTE-DEVICE.md)を確認してください．
