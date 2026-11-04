# ネットワーク実習（Pico W）

[English](../../en/literacy/NETWORKING.md) | [学習ガイド](LEARNING-PATH.md) | [ネットワーク仕様](../reference/devices/NETWORK.md) | [モニタリング実習](MONITORING.md)

この実習では，**Wi-Fiへの接続，IPアドレス，リモートシェル，TCP通信とその観察**を順に学びます．**Pico Wが必要**です．通常のPicoとUSB接続だけではネットワーク通信の実習はできません．最初は既定のAPモードで，インターネットから切り離した実験用ネットワークを使います．

**安全上の注意：** PicoNIXのTelnetとHTTPは暗号化されず，HTTPは認証もありません．大学や公共の無線LAN，インターネットへ公開しないでください．APの既定パスワードは公開値なので，機密情報は送信しないでください．GPIOやPWMを動かすHTTP APIもあるため，安全な機器以外は接続しないでください．

## 1．Pico Wを準備する

[セットアップ](../setup/SETUP.md)に従い，Linux PCでPico W用ファームウェアをビルドして書き込みます．以下のコードブロックはPicoNIX側で実行します．APモードでのSSIDは個体ごとに異なります．実際のSSIDを起動表示で確認し，PCまたはスマートフォンをそのAPに接続します．

```text
piconix# net
piconix# ip addr
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

接続後，HTTPリクエスト`GET / HTTP/1.0`と空行を送ると，サーバから応答が返ることがあります．端末クライアントで改行の送られ方が異なるため，ここは**発展課題**とします．`nc`とICMP Echo専用の`ping`はどちらもPicoNIX上で動作します．

## 6．`tcpdump`でIPv4パケットを観察する

`tcpdump`はPico W専用の**パケット観察コマンド**です．PicoNIX自身のIPv4通信からTCP・UDP・ICMPの概要を表示します．Linuxの`tcpdump`と違い，Ethernetフレームや他の機器同士の通信は取得せず，pcapファイルの保存もできません．

### コマンドの使い方

```text
piconix# tcpdump             # 概要を連続表示（Ctrl+Cで終了）
piconix# tcpdump -c 10       # 10パケット表示して終了
piconix# tcpdump -x          # データ長と先頭最大16バイトを追加表示
piconix# tcpdump -x -c 10    # 両方を組み合わせる
```

`-c COUNT`は表示するパケット件数を指定します．`-x`では，1行目の`DATA=`にデータ長を追加し，データが存在するときだけ，字下げした2行目に16進数とASCIIを表示します．

### 実習：HTTP通信を観察する

1．**PicoNIXのUSB端末**で`tcpdump -x -c 20`を実行し，パケットを待ち受けます．

2．**APに接続したLinux PC**の別の端末で，`curl http://192.168.4.1/`を実行します．`curl`はPC側のコマンドです．PicoNIX側ではありません．必要ならPCのブラウザで同じURLを開いても構いません．`httpd`が起動していなければPicoNIX側で`ps`を確認してください．

3．USB端末の記録を確認します．次は**表示形式を説明するための例**であり，アドレス，ポート，長さ，フラグは通信によって変わります．

```text
RX TCP 192.168.4.2:52000 > 192.168.4.1:80 LEN=60 DATA=16 ACK PSH
    DATA:  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 30 0d 0a | GET / HTTP/1.0..|
RX TCP 192.168.4.2:52000 > 192.168.4.1:80 LEN=40 DATA=0 ACK
```

`RX`は受信，`TX`は送信です．`LEN`はIPv4パケット全体のバイト数，`DATA`はIP・TCP/UDPなどのヘッダを除いたデータのバイト数です．2行目は先頭**最大16バイト**だけを表示し，16進数の各バイトと右側のASCII文字が対応します．表示できない制御文字は`.`に置き換えます．16バイトより短いデータでもASCII欄の位置は揃います．`DATA=0`の場合は2行目を表示しません．また，データ長が16より大きくても表示するのは先頭16バイトだけです．

**観察のポイント：** TCPの`SYN`，`ACK`，`PSH`などのフラグを探し，接続開始，データ転送，応答の順序を考えましょう．`SYN`や純粋な`ACK`には通常データがありません．通信や端末の幅によっては表示が折り返されることがあります．

### 実習：ICMP Echoを観察する

USB端末で`tcpdump -x -c 10`を起動したまま，**別のPicoNIX端末**（Telnetなど）で次を実行します．

```text
piconix# ping -c 2 192.168.4.1
```

このアドレスはAPモードでのPico W自身のIPアドレスの例です．実際のIPアドレスは`ip addr`で確認してください．外部PCを調べるときは，そのPCに割り当てられたIPアドレスを使います．`TYPE=8`はEcho Request，`TYPE=0`はEcho Replyです．送信・受信の両方が記録されるか比較してください．

**注意：** キャプチャ開始前のパケットは表示されません．記録は16件のリングバッファなので，通信量が多いと古い記録が上書きされます．`-c`の件数に達するまで通信がなければ待ち続けるため，Ctrl+Cで終了できます．自分で管理する隔離された実習ネットワークだけで実施してください．詳細は[ネットワーク仕様](../reference/devices/NETWORK.md)を参照してください．

## 振り返り

- APとSTAでは，Pico Wはどちらの役割を担うでしょうか．
- TelnetとHTTPはどちらもTCPを使うのに，ポート番号が異なるのはなぜでしょうか．
- `net`，`lsof`，`/proc/net/memory`は通信のどの側面を示すでしょうか．
- PCで通信を試した時刻とPicoNIXの観察結果を記録し，変化の有無を考察してください．

プログラムからTCP/UDPを使う場合は[Cでのネットワークプログラミング](../development/apps/c/NETWORK-PROGRAMMING.md)へ進みます．デバイスの遠隔制御は認証のない別サービスなので，利用前に[安全上の注意と仕様](../reference/devices/REMOTE-DEVICE.md)を確認してください．

---

## 学習ナビゲーション

**補助教材**です．「前へ」は関連する本編，「次へ」は本編の次のテーマを示します．

| ← 前へ | ↑ 目次へ | 次へ → |
| :--- | :---: | ---: |
| [デバイスI/O](DEVICE-IO.md) | [中級コース](INTERMEDIATE.md) | [上級コース](ADVANCED.md) |
