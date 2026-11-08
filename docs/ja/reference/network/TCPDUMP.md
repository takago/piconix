# `tcpdump`：Pico W自身の通信を観察する

PicoNIXの`tcpdump`は，Pico Wが送受信する**IPv4パケットの要約**を表示します．Linuxの同名コマンドと異なり，ネットワーク全体を盗聴したりpcapを保存したりする機能はありません．観測用の`/dev/net/packet`デバイスから，TCP・UDP・ICMPの情報を取得します．

## コマンドとオプション（PicoNIX側）

```text
piconix# tcpdump
piconix# tcpdump -c 10
piconix# tcpdump -x -c 20
```

| 指定 | 動作 |
|---|---|
| 指定なし | Ctrl+Cまで待機・表示 |
| `-c COUNT` | 指定件数表示後に終了（0以外の整数） |
| `-x` | データ長と最大16バイトの先頭データを16進数＋ASCIIで表示 |

**画面に何も出ない場合でも故障とは限りません．** Pico W自身に向かう通信がなければ待ち続けます．通信が多いと16件のリングバッファが上書きされ，取りこぼす場合があります．ARPや他機器同士のパケットは対象外です．

## 最初の実行例：HTTPの通信を観察

1. Pico WにUSBコンソールをつないで`ip addr`を確認する．
2. PicoNIX側のUSBコンソールで`tcpdump -x -c 20`を起動し，そのまま待つ．
3. 同じネットワーク上の**Linux PC**で`curl http://192.168.4.1/`を実行する（IPはAPモードの一例）．PC側は`curl`を使用し，PicoNIX側には`curl`はありません．
4. PicoNIXの`tcpdump`画面で`RX TCP`（受信）や`TX TCP`（送信）の行を探す．
5. `-c 20`で終了しなければ，通信量が足りない可能性があります．必要に応じて`Ctrl+C`で終了します．

**説明用の想定出力（実測ではありません）：**

```text
RX TCP 192.168.4.2:52000 > 192.168.4.1:80 LEN=60 DATA=16 ACK PSH
    DATA:  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 30 0d 0a | GET / HTTP/1.0..|
```

## 各フィールドの意味

| 表示 | 何を表すか |
|---|---|
| `RX` / `TX` | Pico Wから見た受信／送信方向 |
| `TCP` / `UDP` / `ICMP` | IPv4上位のプロトコル |
| `192...:52000 > ...:80` | 送信元IP・ポートから宛先IP・ポートへ（ICMPにはポートなし） |
| `LEN=` | IPパケット全体の長さ（ペイロード長だけではない） |
| `DATA=` | TCP/UDPデータの長さ．`-x`指定時に表示 |
| `SYN` / `ACK` / `FIN` / `RST` / `PSH` | TCP制御フラグ（接続，確認，終了，異常，データ） |
| `TYPE=8` / `TYPE=0` | ICMP Echo Request／Echo Reply |
| 2行目の`DATA:` | 先頭最大16バイトの内容．通信全体ではない |

`DATA`は**先頭フラグメントの観察範囲**に関する値です．`tcpdump`ではイーサネット全フレームやTLSの復号情報は得られません．

## 比較操作例：ICMP，TCP，UDP

- ICMP：2つ目のPicoNIX端末から`ping -c 2 192.168.4.1`を実行して`TYPE=8/0`を探す（[ping](PING.md)）．
- TCP：PC側で`curl`を実行し`SYN`，`ACK`，`FIN`の順序を考える．
- UDP：PC上の`udp_echo_host.py`とPico Wの`udp_once`を使い`UDP`の往復を観察する（[ソケット実習](../../development/apps/c/NETWORK-PROGRAMMING.md)）．

**重要：** `-x`はデータの一部を表示するため，個人情報やパスワードの流れる環境で実行しないでください．Telnetも平文なので，隔離した実習ネットワークで使ってください．


---

[ネットワーク目次](README.md) ｜ [ping](PING.md) ｜ [ソケット実習](../../development/apps/c/NETWORK-PROGRAMMING.md) ｜ [English](../../../en/reference/network/TCPDUMP.md)
