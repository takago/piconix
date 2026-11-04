# ネットワーク — 技術リファレンス

Pico WのWi-Fi接続，TCP/UDP通信，ネットワークサービスに関する仕様をまとめています．Pico Wがない場合，ネットワーク関連の学習章は選択です．

## 通信を調べるコマンド（初めて使う人向け）

`ip`，`ping`，`tcpdump`は**違う層**を観察します．次の順序で試すと，原因を切り分けやすくなります．

1. [`ip`：自分のIPv4アドレス・インターフェースを確認](IP.md)
2. [`ping`：ICMP Echoで通信可能か確認](PING.md)
3. [`tcpdump`：TCP・UDP・ICMPのパケット要約を読む](TCPDUMP.md)
4. [`net`：Wi-Fi状態・AP接続端末・ARPを確認](NET.md)

実習は[教科書B6](../../learn/B/B6.md)から始められます．**Pico W専用**の操作とPC側で実行する操作を混同しないでください．


## 接続と状態確認

- [Pico Wのネットワーク接続](NETWORK.md)
- [Wi-Fiモードと状態表示](WIFI-MODES.md)
- [`net`によるネットワーク診断](NET.md)

## アプリケーションとサービス

- [`nc`（netcat）と通信の終了](NC.md)
- [ユーザ空間HTTPサーバ（`httpd`）](HTTPD.md)
- [UDP 2000番の遠隔デバイス制御](REMOTE-DEVICE.md)
- [UDPによるGPIO遠隔制御](REMOTE_GPIO.md)

**関連する開発資料：** [ユーザCアプリ開発](../../development/apps/c/NETWORK-PROGRAMMING.md) ｜ [カーネル側ネットワーク実装・拡張](../../development/network/NETWORK-DEVELOPMENT.md)．

**学ぶ順序を確認：** [B6 ネットワーク](../../learn/B/B6.md) → [C5 ネットワーク実装](../../learn/C/C5.md)．

---

**関連リンク：** [リファレンス総目次](../README.md) ｜ [デバイス](../devices/README.md) ｜ [教科書](../../learn/README.md) ｜ [English](../../../en/reference/network/README.md)
