# Pico W DHCP実装・障害調査メモ

この文書は利用者向けの接続方法ではなく，PicoNIXのDHCPサーバの実装で判明した重要事項をまとめた保守用記録です．通常の利用方法は[NETWORK.md](NETWORK.md)を参照してください．

## 実機で確認した現象

- LinuxクライアントのDHCP DISCOVERが最初はDHCP受信コールバックへ届かなかった．
- Pico SDK 2.1.1に含まれるlwIPの`ip4.c`では，宛先IPが未設定のDHCPパケットを受け入れる条件に注意が必要だった．PicoNIXはDHCP**サーバ**なので`LWIP_DHCP=0`である．`kernel/lwipopts.h`に`#define LWIP_IP_ACCEPT_UDP_PORT(port) ((port) == PP_NTOHS(67))`を設定してUDP 67番宛てを受信可能にした．
- DISCOVERを受信してOFFERを組み立てるところまでは正常だった．`pbuf_alloc()`，`pbuf_take()`，`pbuf_free()`のみではフリーズしなかった．
- DHCP受信コールバック（`cyw43_arch_poll()`内で呼ばれる）から直接`udp_sendto()`を実行すると，`ERR_OK`が返り，pbuf解放とコールバック復帰まで進んでも，`cyw43_arch_poll()`が戻らず停止した．ブロードキャスト・ユニキャストの両方で発生した．
- 送信するOFFER/ACKを一旦保存し，`cyw43_arch_poll()`から戻った後で`udp_sendto()`を実行すると停止せず，LinuxでDHCPによるIPアドレス取得に成功した．

## 実装上の方針

受信コールバックではパケットの解析と応答の準備にとどめ，実際のUDP送信はポーリング処理の外で行う．既存のTCPシェルも受信をキューに格納し，送信は後段の処理で行う．単一クライアント向けの設計とする．

## 原因についての注意

コールバック内のUDP送信で停止した**内部的な原因は未確定**．lwIP Raw API一般がコールバック内送信を禁止しているわけではない．CYW43ドライバとの相互作用やPicoNIX固有の条件は今後の調査対象．回避策は実機で確認済みだが，デッドロックと断定しないこと．

## 関連する設定

Telnet（TCP 23）と生TCP（TCP 2323）を同時に待ち受けるため，`kernel/lwipopts.h`の`MEMP_NUM_TCP_PCB_LISTEN`は2以上必要．同時に使用するネットワークTTYは1接続のみ．
