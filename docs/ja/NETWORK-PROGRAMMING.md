# ソケットAPIとネットワークアプリケーション

[English](../en/NETWORK-PROGRAMMING.md) | [ネットワーク接続](NETWORK.md)

## 構成

Pico WではlwIPをカーネル側で利用し，ユーザプロセスに小さなIPv4ソケットAPIを公開します．`net_socket.c`はソケットバックエンド，`net.c`はWi-Fi管理，`net_service_telnet.c`と`net_service_remote_devctl.c`はカーネル標準サービスです．`nc`，`httpd`，`tcp_echod`，`udp_echod`はユーザプロセスとして動作します．通常のPicoではWi-Fi通信を利用できません．

## 利用できるAPI

- TCP：`socket(AF_INET, SOCK_STREAM, 0)`，`bind()`，`listen()`，`accept()`，`connect()`，`send()`，`recv()`，`close()`
- UDP：`socket(AF_INET, SOCK_DGRAM, 0)`，`bind()`，`sendto()`，`recvfrom()`，`close()`
- 接続済みTCPソケットには`read()`と`write()`も利用可能です．`send()`と`recv()`はflags=0のみ対応し，既存の入出力処理を再利用します．

**制限：** 完全なBSD/POSIX互換ではありません．一部操作は非ブロッキングで，データ未到着時は`OS_EAGAIN`を返します．エラーは負の`OS_*`値として返されます．TCPの`bind()`は待受を開始し，`listen()`は互換用の処理です．`connect()`の完了は`tcp_status()`で確認します．DNS，`getsockname()`，一般的なソケットオプションなどはありません．

## ユーザプログラムの例

| コマンド | 役割 | ポート |
|---|---|---|
| `nc` | TCP/UDPクライアント・サーバのテスト | 指定値 |
| `httpd` | `/etc/index.html`を配信するHTTP/1.0サーバ | TCP 80 |
| `tcp_echod [PORT]` | 受信したバイト列を返すTCP Echoサーバ | TCP 7（既定） |
| `udp_echod [PORT]` | 受信したデータグラムを送信元へ返すUDP Echoサーバ | UDP 7（既定） |

PicoNIX側で`tcp_echod 1234`を起動し，Linuxから`nc 192.168.4.1 1234`で接続できます．UDPならPicoNIXで`udp_echod 1234`を起動し，Linuxから`printf 'hello\n' | nc -u -w 2 192.168.4.1 1234`を実行します．同一のポート7を使う場合もTCPとUDPは別プロトコルです．

`httpd`を起動すれば`http://192.168.4.1/`でページを表示できます．`edit /etc/index.html`で内容を変更できます．`lsof`ではソケットとローカルポートを確認できます．

## カーネル標準サービス

- TCP 23：Telnetによるシェル操作．
- UDP 2000：`READ`，`WRITE`，`ENABLE`，`DISABLE`による許可されたGPIO・ADC・PWM・フレームバッファ操作．

UDP 2000の例：`printf 'READ /dev/adc/temp\n' | nc -u -w 2 192.168.4.1 2000`．認証・暗号化はないため，信頼できる実験ネットワークに限定してください．

## 実装の見どころ

カーネルのネットワークサービスと，ソケットAPIを使う通常のユーザアプリケーションを区別しています．`trace`でシステムコールを，`lsof`でソケットのFDとポートを観察できます．開発の経緯を記録した`docs/en/NETWORK_STAGE*.md`などは履歴資料です．現在の仕様は本書を優先してください．
