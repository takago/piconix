# `net`：Wi-Fi・AP・ARPの診断

`net`はPico Wのネットワーク**状態**を観察するコマンドです．`ip`がインターフェースのIPv4アドレスを要約するのに対し，`net`は`/proc/net/`以下の情報をそのまま表示します．**通信相手への疎通確認ではありません**．

## 実行例（PicoNIX側）

```text
piconix# net
piconix# net -a
piconix# net -r
piconix# net -h
```

| コマンド | 読み取る情報 | 利用場面 |
|---|---|---|
| `net` | `/proc/net/wifi`：モード，接続状態，IP，MAC，エラー履歴等 | Wi-Fiが接続されているか |
| `net -a` | `/proc/net/ap`：AP接続クライアント情報 | APへPCが接続したか |
| `net -r` | `/proc/net/arp`：ARPのIP・MAC対応 | 最近通信した相手のIPv4とMAC |
| `net -h` | コマンドの使い方 | オプション確認 |

**観察手順：** (1) PicoNIX側で`net`を実行し`MODE`と`STATUS`を確認．(2) APモードならPC接続後に`net -a`を確認．(3) PCからPico WへHTTPアクセスして`net -r`を確認．(4) 取得したIPを使って[`ping`](PING.md)で疎通確認．

`net -r`に相手が表示されない場合も，ARPキャッシュが空である可能性があります．表示されるIP・MACは環境依存です．`net`自体はパケットを発生させないため，`tcpdump`の代わりにはなりません．通常のPicoではWi-Fi関連情報は利用できません．

**調べ分け：** Wi-Fiの接続は`net`，自分のIPv4は[`ip`](IP.md)，ICMP疎通は[`ping`](PING.md)，TCP/UDPパケットは[`tcpdump`](TCPDUMP.md)．

---

[ネットワーク目次](README.md) ｜ [Wi-Fi接続仕様](NETWORK.md) ｜ [English](../../../en/reference/network/NET.md)
