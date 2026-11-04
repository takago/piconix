# net：ネットワーク診断

> **資料区分：技術リファレンス**．仕様を調べるためのページです．[リファレンス目次](../README.md) ｜ [教科書総目次](../../learn/README.md)．

`net`はPico WのWi-Fi/IP/DHCP状態を表示するユーザコマンドです．Pico（Wi-Fi非搭載）では利用可能なネットワーク情報はありません．

```sh
net       # /proc/net/wifi の基本情報
net -a    # /proc/net/ap のAP接続端末MAC一覧
net -r    # /proc/net/arp のARPキャッシュ
```

STAモードでは`DHCP: Client`，APモードではDHCPサーバの稼働状況を表示します．`net -a`はSTAモードで利用できません．`net -r`はAP/STA両方で利用できます．

接続端末の一覧とARPは異なります．ARP表には通信した相手しか現れず，切断後もしばらく残る場合があります．PicoNIXのAP DHCPサーバは単一のリースに制限されているため，AP接続端末が複数あっても全端末にDHCPアドレスが払い出されるとは限りません．

カーネルはドライバの関連付け情報をネットワーク処理側で約1秒ごとに収集し，システムコール内ではWi-Fiドライバに直接問い合わせません．`/proc/net/ap`と`/proc/net/arp`は読み取り専用です．
