# Wi-Fiモードと状態表示

*技術リファレンス*

Pico WではAPモードが既定です．`PicoNIX-XXXXXXXX`というアクセスポイントを提供し，1台の端末にDHCPでアドレスを割り当てます．既存のTelnet（TCP 23），遠隔デバイス制御（UDP 2000），ユーザプロセス向けソケット機能はそのまま使用します．

STAモードでは，既存のWi-Fiアクセスポイントに接続します．

```sh
cmake -S . -B build-sta -DPICO_BOARD=pico_w \
  -DPICONIX_WIFI_MODE=STA \
  -DPICONIX_WIFI_SSID='YourNetwork' \
  -DPICONIX_WIFI_PASSWORD='YourPassword'
```

STAモードではDHCPクライアントを利用します．接続失敗時にはエラーを表示し，APへ自動切替しません．モードごとに別のビルドディレクトリを使用してください．

**注意**：パスワードはファームウェアとCMakeキャッシュなどに残ります．ビルドディレクトリや認証情報入りUF2を公開しないでください．TelnetとUDPデバイス制御には強固な認証・暗号化がないため，信頼できるネットワークだけで使用してください．

```sh
cat /proc/net/wifi
```

`MODE`，`STATUS`，`SSID`，`ATTEMPTS`，`CONNECT_ERROR`，`WIFI_LINK`，`TCPIP_LINK`，`DHCP_BOUND`，`IP`，`NETMASK`，`GATEWAY`などを表示します．表示項目はモードや接続状態によって異なります．Wi-Fi非対応のPicoビルドでは`MODE: N/A`を返します．


STAモードでは，Wi-Fiの参加状態だけでなく，TCP/IPリンク状態とDHCPのアドレス取得状況を使って接続完了を判定します．

## STA通信の診断

STAモードでは，STAインターフェースの有効化後に`CYW43_NONE_PM`を設定して，
Wi-Fiの省電力機能を無効化します．APモードの設定は変更しません．
CYW43/lwIPを実行するアイドルタスクのスタックは4 KiBです．

STAモードの`/proc/net/wifi`には，次の診断項目も表示します．

- `PM_ERROR`：省電力無効化の設定結果です．`0`が成功を表します．
- `POLL_COUNT`：起動後に`net_poll()`を呼んだ回数です．
- `POLL_MAX_GAP_MS`：`net_poll()`の呼び出し間隔の最大値（ミリ秒）です．パケット数ではありません．
- `IDLE_STACK`：アイドルスタックの使用量の概算最大値／確保容量（バイト）です．4096に近ければ注意が必要です．

スタック使用量は未使用領域のマーカーを調べた概算値であり，
オーバーフローがなかったことを保証するものではありません．
起動処理用スタックの使用量は含みません．

`STATUS: UP`やDHCPによるIPアドレス取得は，ARP，ICMP，Telnetの通信成功を保証しません．LinuxなどのホストPCから`ping`で疎通を調べるほか，Pico W版のPicoNIXでも`ping -c 2 PCのIPv4アドレス`を実行できます（[ping](PING.md)を参照）．通常のPico版ではこのICMP機能は利用できません．
電源の入れ直しを伴う複数回の接続試験とLinux側でのARP/ICMP観測を行ってください．ビルド時にはカーネルRAMの56 KiB制限を確認してください．実機での通信安定性は引き続き検証が必要です．


## MACアドレスと受信強度インジケータ

`net`および`cat /proc/net/wifi`は，Pico WのWi-Fiインターフェース自身の
`MAC`アドレスをAP・STA両モードで表示します．CYW43への問い合わせは
ネットワークのポーリング処理で行い，結果を保持します．取得前または
取得失敗時には`N/A`を表示します．ルータのMACアドレスではありません．

STAモードでは，接続中に約5秒間隔でRSSI（受信信号強度）を取得します．
表示例は次のとおりです．

```text
MAC: 28:CD:C1:12:34:56
RSSI: -44 dBm (100%)
```

百分率は**RSSIから換算した受信強度の目安**であり，通信品質や速度の実測値ではありません．
`-100 dBm = 0%`，`-50 dBm以上 = 100%`とし，その間を線形換算して
0～100%に収めます（整数演算）．例えば`-67 dBm`は`66%`になります．
受信強度だけでは，電波干渉，再送，パケット損失などは判断できません．

`RSSI: N/A`は，未接続・取得失敗・初回測定前など，測定値が有効でない
状態を意味します．APモードでは接続先アクセスポイントの受信強度を
測定するわけではないため，`RSSI: N/A (AP mode)`と表示します．
CYW43への問い合わせはアイドルタスク内で行い，`/proc`読み取り中には
実行しません．

## mDNSによる名前解決

Pico W版はlwIPのmDNSレスポンダを使用します．STAモードではDHCPによるIPアドレス取得後に，Flash IDを小文字の16進数8桁にした`piconix-xxxxxxxx.local`を登録します（例：`piconix-4353ba2e.local`）．APモードでは固定名`piconix.local`を使用し，AP側のIPアドレスは`192.168.4.1`です．

```sh
# STAモード（Flash IDは実機の値に置き換える）
avahi-resolve-host-name piconix-4353ba2e.local
telnet piconix-4353ba2e.local

# APモードに接続した端末から
telnet piconix.local
```

`/proc/net/wifi`の`MDNS_NAME`はホスト名，`MDNS_ACTIVE`はレスポンダの登録状態，`MDNS_ERROR`は登録時のエラー値を表します．mDNSは名前をIPアドレスへ解決する機能であり，ARP（IPアドレスからMACアドレスへの解決）の代わりにはなりません．`.local`の名前解決にはクライアント側のmDNS対応が必要です．

## STAモードのGratuitous ARP（暫定機能）

STAモードではDHCP取得後，60秒間隔でGratuitous ARP（GARP）を送信する実験的な処理を行います．`GARP_INTERVAL_S`は間隔，`GARP_OK`はlwIP送信APIの成功回数，`GARP_ERROR`は失敗回数を示します．送信APIの成功は，無線送信の完了や相手側への到達を保証しません．APモードではこの定期送信を行いません．

GARPの到着間隔はアクセスポイントやクライアントの構成に左右されます．送信時刻と受信時刻を区別して確認してください．定期GARPはARP接続障害の根本的な解決を保証しません．


AP・STAどちらでも`cat /proc/net/memory`で追加のメモリ診断を確認できます．

---

**関連リンク：** [リファレンス目次](../README.md) ｜ [English](../../../en/reference/network/WIFI-MODES.md)
