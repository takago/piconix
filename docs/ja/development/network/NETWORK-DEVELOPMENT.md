# Pico Wネットワーク実装・拡張ガイド

> **資料区分：開発ガイド（教科書・API仕様ではありません）**．[ネットワーク開発目次](README.md) ｜ [教科書C5](../../learn/C/C5.md) ｜ [English](../../../en/development/network/NETWORK-DEVELOPMENT.md)

この資料は，**カーネル側**のWi-Fi，lwIP，ソケット，常駐サービスをソースから追い，安全に変更するための道順です．Cアプリからの`socket()`などの呼出方法は[ユーザ向けソケットAPI](../apps/c/NETWORK-PROGRAMMING.md)，実際の接続方法・制限値は[ネットワークリファレンス](../../reference/network/README.md)に集約します．**ネットワークは[デバイスドライバ開発](../drivers/DRIVER-DEVELOPMENT.md)の後**に学ぶ選択分野です．

## 1．最初に理解する分担

| 担当 | ソースファイル | 確認する点 |
|---|---|---|
| 起動・アイドル実行 | [`kernel/main.c`](../../../../kernel/main.c) | `net_init()`，`idle_task()`内の`net_poll()` |
| Wi-Fi・lwIP初期化とポーリング | [`kernel/net.c`](../../../../kernel/net.c)，[`kernel/net.h`](../../../../kernel/net.h) | AP/STA，DHCP，mDNS，ポーリング |
| ユーザソケットのバックエンド | [`kernel/net_socket.c`](../../../../kernel/net_socket.c)，[`kernel/net_socket.h`](../../../../kernel/net_socket.h) | TCP/UDP状態，受信，送信，接続終了 |
| システムコール・FDとの接続 | [`kernel/kernel.c`](../../../../kernel/kernel.c) | ソケットFDの確保・解放，SVCからバックエンドへの委譲 |
| 常駐リモートシェル | [`kernel/net_service_telnet.c`](../../../../kernel/net_service_telnet.c) | TCP 23，ネットワークTTY，切断時の処理 |
| 常駐デバイス制御 | [`kernel/net_service_remote_devctl.c`](../../../../kernel/net_service_remote_devctl.c) | UDP 2000，要求の解析・処理 |
| パケット観察・ICMP | [`kernel/net_capture.c`](../../../../kernel/net_capture.c)，[`include/piconix_net_capture.h`](../../../../include/piconix_net_capture.h) | 観測レコード，Echo要求・応答 |
| lwIP設定 | [`kernel/lwipopts.h`](../../../../kernel/lwipopts.h) | メモリプール・TCP設定 |
| 比較用のユーザサービス | [`userland/apps/httpd.c`](../../../../userland/apps/httpd.c) | `httpd`はカーネル常駐ではない |

`kernel/net_services.h`には常駐サービスの内部入口があり，`include/piconix.h`などと合わせて境界を確認できます．関数名の検索には`rg -n 'net_init\(|net_poll\(|net_socket_|net_service_' kernel/`が使えます．

## 2．読む順序と二つのデータ経路

**先に共通の流れを追う：** `main()` → `net_init()` → `idle_task()` → `net_poll()`．Pico Wでは，アイドルタスクが`cyw43_arch_poll()`やネットワークサービスの処理機会を作ります．`net_poll()`は割込みハンドラそのものではなく，最短呼出間隔も一定の実行周期を保証しません．詳細な設定値と観察項目は[ネットワーク接続リファレンス](../../reference/network/NETWORK.md)を参照してください．

次に，次の二つを**混同せず**追ってください．

1. **ユーザTCP/UDP：** [`userland/apps/httpd.c`](../../../../userland/apps/httpd.c)など → [`userland/lib/syscalls.c`](../../../../userland/lib/syscalls.c) → `kernel/kernel.c`のSVC・FD処理 → `net_socket.c` → lwIP．`httpd`の起動・終了はユーザプロセスの仕組みで扱います．
2. **カーネル常駐サービス：** APモードでは`net_init()`，STAモードでは接続成立後の`sta_start_services()`が準備するTelnet（TCP 23）・遠隔デバイス制御（UDP 2000） → `net_service_*.c`のlwIPコールバック／ポーリング → 必要なTTY・デバイス処理．ユーザアプリのFD経路と同一視しないでください．

`net_capture.c`は観察・ICMP処理の別の入口です．パケット観測レコードは生のEthernetフレームではありません．

## 3．変更する目的からファイルを選ぶ

| 変更したいもの | 主に読む場所 | 変更後に確認すること |
|---|---|---|
| AP/STA接続，再接続，DHCP，mDNS | `net.c` | 接続の再試行，Telnetの存続，`/proc/net/wifi` |
| ポーリングの順序・間隔 | `net.c`，`main.c` | アイドル処理，端末応答，`POLL_MAX_GAP_MS` |
| TCP/UDPの送受信・ソケット寿命 | `net_socket.c`，`kernel.c` | EOF，半閉鎖，FD解放，再起動後の再利用 |
| Telnet入力とネットワークTTY | `net_service_telnet.c`，`kernel.c` | USBコンソールとの独立性，接続終了・再接続 |
| UDPデバイス制御 | `net_service_remote_devctl.c` | 許可パス，入力長，コールバック中の処理 |
| `ping`・`tcpdump`の観察 | `net_capture.c`，`include/piconix_net_capture.h` | レコード形式，バッファ枠，取りこぼし |
| ユーザ用ソケットAPIの追加 | `kernel.c`，`net_socket.c`，`userland/lib/syscalls.c` | [システムコール仕様](../../reference/SYSCALLS.md)，ABI，ユーザAPI |

## 4．壊してはいけない境界

- **ポーリング文脈：** lwIP raw APIのコールバックから再入や同期的な送信を安易に行わず，既存の保留処理・ポーリングとの役割を確認します．UDP応答やループバックは特に注意してください．
- **バッファの所有権：** TCP受信時のpbufと`ERR_MEM`，`tcp_recved()`，送信キューと`tcp_write()`の結果を確認します．データの解放・二重送信・ACK処理を変える前に[現行のTCP動作](../../reference/network/NETWORK.md)を読んでください．
- **FDとプロセスの寿命：** タスク終了・ソケットclose・TCPリスナの終了を通じてカーネル側の管理枠が解放されることを確認します．
- **メモリ制約：** TCP枠，pbufプール，lwIPヒープやアイドルスタックはRAMを使います．変更前後に`/proc/net/memory`とビルド後のメモリ報告を比較します．
- **対象機種：** `PICONIX_WIFI`による通常Pico向けの無効実装を維持し，Pico Wだけでなく通常Picoもビルド可能にします．
- **セキュリティ：** TelnetとUDP 2000の制御は認証・暗号化を前提としていません．信頼できる隔離した実習ネットワークでのみ検証してください．

## 5．改造前後の検証手順

まずカーネルを変更せず，現状の挙動を記録します．必要なら[教科書B6](../../learn/B/B6.md)の実習を利用してください．

```sh
# 開発PCのリポジトリ直下で実行する．ソース回帰テスト
python3 tools/run_tests.py

# SDKがある場合，書き込みを行わずPico W用にビルドする
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
  ./tools/build_sdk.sh --no-flash -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel

# 通常Picoにも影響しないか確認
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
  ./tools/build_sdk.sh --no-flash -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

実機に書き込む場合は，通信の変更を最小単位で適用し，USBコンソールなど別の観察経路を確保してください．Pico W上では次を確認します．

```sh
cat /proc/net/wifi
cat /proc/net/memory
net
lsof
```

AP接続，STA接続（使う場合），Telnet接続／切断，`httpd`や`nc`などのユーザソケット，`127.0.0.1`宛てループバック，通信の繰返しをそれぞれ確認します．`tcpdump`と`ping`はパケット観察に使えますが，実機がなくてもソース読みは可能です．**回帰テストとビルドの成功だけでは実機の通信安定性は保証されません．**

## 6．文書の更新先

- **利用者に見える接続方法・制約・診断値**： [技術リファレンス](../../reference/network/README.md)
- **ユーザCプログラムからの利用方法・ソケット互換性**： [ネットワークアプリ開発](../apps/c/NETWORK-PROGRAMMING.md)
- **カーネルの読解手順・変更の注意点**： 本ガイド
- **学習順序・確認課題**： [教科書C5](../../learn/C/C5.md)

実装と文書が食い違った場合は，現行ソースを確認し，関連する**日本語・英語の両方**のページを修正してください．
