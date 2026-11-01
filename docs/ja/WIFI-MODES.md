# Wi-Fiモードと状態表示

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
