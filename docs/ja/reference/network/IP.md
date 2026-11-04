# `ip`：IPアドレスとインターフェースの確認

PicoNIXの`ip`は**読み取り専用の小さな状態表示コマンド**です．Linuxの`iproute2`とは異なり，アドレスや経路を変更する`ip addr add`，`ip route`などの機能はありません．Pico Wで通信を始める前に，Pico W自身のアドレスとWi-Fi接続状態を調べるために使います．

## 基本操作（PicoNIX側）

```text
piconix# ip
piconix# ip addr
piconix# ip -h
```

`ip`と`ip addr`は同じ概要を表示します．表示内容は`/proc/net/wifi`から取り出しています．通常のPicoではWi-Fiインターフェースがない旨が表示されます．

**APモードの想定例（実測ではありません）**：

```text
lo0: 127.0.0.1/8 (loopback)
wifi: AP (UP)
  inet 192.168.4.1  netmask 255.255.255.0
  ether xx:xx:xx:xx:xx:xx
```

実際のインターフェース名・状態・アドレスはファームウェアと接続状況に依存します．`UP`は表示用のネットワーク状態であり，**アプリのTCP待受やPCからの接続成功を保証しません**．

## 各項目の読み方

| 項目 | 意味 | 次に確認すること |
|---|---|---|
| `lo0` | ループバック（自分自身への通信） | `127.0.0.1`はPico W自身 |
| `wifi`/`IFACE` | Wi-Fiのインターフェース | モードAP/STAを確認 |
| `AP` / `STA` | アクセスポイント／既存LAN参加 | PCが同じ通信可能なネットワークか |
| `inet` | Pico WのIPv4アドレス | PCのアドレスと混同しない |
| `netmask` | IPv4のネットマスク | 同一サブネットか |
| `ether` | 無線インターフェースのMAC | IPアドレスとは別 |

## 段階的な切り分け

1. PicoNIX側で`ip addr`を実行する．
2. `net`で`STATUS`，`GATEWAY`，RSSI等を調べる（[netガイド](NET.md)）．
3. **Linux PC側**で`ip addr`を実行し，PCのIPアドレスを調べる．
4. PicoNIX側で`ping -c 2 PCのIPv4アドレス`を実行する（[pingガイド](PING.md)）．
5. `ping`が成功してもTCP 80などのサービスが起動しているとは限らない．`ps`や`lsof`でサーバを調べる．

**確認問題：** `lo0`とWi-FiのIPはどう違いますか．`inet`に表示されているのはPico W側とPC側のどちらのアドレスですか．

---

[ネットワーク目次](README.md) ｜ [ping](PING.md) ｜ [English](../../../en/reference/network/IP.md)
