# `ping`：ICMP Echoで疎通を確かめる

`ping`はIPv4宛てに**ICMP Echo Request**を送り，Echo Replyが返るか調べるコマンドです．PicoNIXの`ping`はPico W専用で，Linuxのすべての`ping`オプションには対応しません．カーネルの専用デバイス`/dev/net/icmp`を使い，rawソケットは公開しません．

## 使い方（PicoNIX側）

```text
piconix# ping 192.168.4.2
piconix# ping -c 2 192.168.4.2
```

- 宛先は**数値IPv4アドレス**．ホスト名・DNS名前解決は非対応．
- `-c COUNT`は送信回数（1～100）．省略すると4回．
- 1回の応答待機はおおむね1.25秒で，次回の送信まで間隔があります．
- `ping`は`/dev/net/icmp`を排他的に使います．別の`ping`実行中は開けないことがあります．

APモードでは`192.168.4.1`がPico W自身，`192.168.4.2`がPCという構成が一般的ですが，**必ず`ip addr`とPCのIPを先に確認**してください．

## 想定出力の読み方

```text
reply from 192.168.4.2: seq=1 time=4 ms
reply from 192.168.4.2: seq=2 time=5 ms
2 transmitted, 2 received
```

上記は**説明用の架空の値**です．`seq`は要求番号，`time`はEchoの往復時間（ミリ秒）です．`timeout: seq=N`ならその回のEcho Replyを確認できなかったという意味であり，必ずしもWi-Fi自体の故障とは限りません．PC側のファイアウォールがICMPを拒否している場合もあります．

## 疎通確認の手順

1. PicoNIXで`ip addr`，`net`を実行し，Wi-Fiが接続状態であることを確認する．
2. Linux PCでIPアドレスを調べる．
3. PicoNIXから`ping -c 2 PCのIPv4`を実行する．
4. 次に**Pico W自身**のアドレスへ`ping -c 2 192.168.4.1`（APモードの例）を実行する．
5. `tcpdump -c 10`を別のPicoNIX端末で動かしてから`ping`を行い，ICMP `TYPE=8`（要求）と`TYPE=0`（応答）を探す（[tcpdump](TCPDUMP.md)）．

## 失敗時の確認

| 表示・症状 | 原因を分けて考える |
|---|---|
| `usage` | 数値IPv4の形式，`-c`の位置と回数 |
| `ICMP device unavailable or busy` | 通常Pico／専用FDが別タスクに占有されている |
| `send failed` | 送信要求の失敗．Wi-Fi状態とメモリ診断を確認 |
| `timeout` | 応答なし．相手IP，接続，ICMP制限，負荷を順に確認 |
| 自分へは成功，PCへ失敗 | PC側ファイアウォールとAP接続を優先して確認 |

`ping`の成功は**IP層までの疎通を示すだけ**で，HTTPなどのTCP/UDPアプリケーションが使えることまでは証明しません．そこは`nc`やCクライアントで別途調べてください．

---

[ネットワーク目次](README.md) ｜ [ip](IP.md) ｜ [tcpdump](TCPDUMP.md) ｜ [English](../../../en/reference/network/PING.md)
