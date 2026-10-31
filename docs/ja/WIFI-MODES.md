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

`MODE`，`SSID`，`MAC`，`IP`，`NETMASK`，`GATEWAY`，`RSSI`を表示します．STAのRSSIは読み出し時に取得し，APでは`N/A`です．

起動時には`/tmp/test.s`と`/tmp/test.py`も配置されます．どちらも1秒間隔で顔文字を3回表示し，`...bye`で終了します．
