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

## STA通信の診断

STAモードでは，STAインターフェースの有効化後に`CYW43_NONE_PM`を設定して，
Wi-Fiの省電力機能を無効化します．APモードの設定は変更しません．
また，CYW43/lwIPを実行するアイドルタスクのスタックを4 KiBに拡張しました．

STAモードの`/proc/net/wifi`には，次の診断項目も表示します．

- `PM_ERROR`：省電力無効化の設定結果です．`0`が成功を表します．
- `POLL_COUNT`：起動後に`net_poll()`を呼んだ回数です．
- `POLL_MAX_GAP_MS`：`net_poll()`の呼び出し間隔の最大値（ミリ秒）です．パケット数ではありません．
- `IDLE_STACK`：アイドルスタックの使用量の概算最大値／確保容量（バイト）です．4096に近ければ注意が必要です．

スタック使用量は未使用領域のマーカーを調べた概算値であり，
オーバーフローがなかったことを保証するものではありません．
また，起動処理用スタックの使用量は含みません．

`STATUS: UP`やDHCPによるIPアドレス取得は，ARPやping，Telnetの成功を保証しません．
以前のファームウェアとの比較，電源の入れ直しを伴う複数回の接続試験，
Linux側でのARP/ICMPの観測を行ってください．
4 KiBへの増量後も，リンク時のカーネルRAM 64 KiB制限の検証が必要です．
実機での通信安定性が確認されるまでは，今回の変更を原因確定とは扱いません．
