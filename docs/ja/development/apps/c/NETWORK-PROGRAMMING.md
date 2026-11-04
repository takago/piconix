# ソケットAPIとネットワークアプリケーション

*開発ガイド*

## 構成

このガイドは，**ユーザCアプリケーションから使う**IPv4ソケットAPIとプログラム例を扱います．Pico WではlwIPがカーネル側にあり，`nc`，`httpd`，`tcp_echod`，`udp_echod`はユーザプログラムとして動きます．通常のPicoではWi-Fiを利用できません．Pico Wでは`127.0.0.1`宛てのTCP/UDPも利用できます．

カーネル側の`net.c`・`net_socket.c`・常駐Telnet/UDPサービスの読解や改造は[ネットワーク実装・拡張ガイド](../../network/NETWORK-DEVELOPMENT.md)に分離しました．接続条件やメモリ上限は[ネットワーク接続](../../../reference/network/NETWORK.md)と[メモリ配置](../../../reference/MEMORY-LAYOUT.md)を参照してください．

## 利用できるAPI

- TCP：`socket(AF_INET, SOCK_STREAM, 0)`，`bind()`，`listen()`，`accept()`，`connect()`，`send()`，`recv()`，`close()`
- UDP：`socket(AF_INET, SOCK_DGRAM, 0)`，`bind()`，`sendto()`，`recvfrom()`，`close()`
- 接続済みTCPソケットには`read()`と`write()`も利用可能です．`send()`と`recv()`はflags=0のみ対応し，既存の入出力処理を再利用します．

**制限：** 完全なBSD/POSIX互換ではありません．一部操作は非ブロッキングで，データ未到着時は`OS_EAGAIN`を返します．エラーは負の`OS_*`値として返されます．TCPの`bind()`は待受を開始し，`listen()`は互換用の処理です．`connect()`の完了は`tcp_status()`で確認します．DNS，`getsockname()`，一般的なソケットオプションなどはありません．

## BSD風ラッパーの具体的な制限

`userland/lib/syscalls.c`の実装では，`listen(fd, backlog)`は`backlog < 0`なら`OS_EINVAL`を返し，それ以外ではFDの妥当性を確認せず0を返します．実際のTCP待受は`bind()`で開始するため，`listen()`の成功を待受可能の判定に使用しないでください．

`accept(fd, addr, len)`は接続相手のアドレス取得に対応せず，`addr`または`len`が非NULLなら`OS_EINVAL`を返します．現行APIでは`accept(fd, NULL, NULL)`を使用します．

`recvfrom()`では`from`が非NULLの場合，`fromlen`も非NULLで，呼び出し前の`*fromlen`が`sizeof(struct sockaddr_in)`以上である必要があります．受信成功時にIPv4の送信元情報と実際の構造体サイズを格納します．`from == NULL`の場合は送信元アドレスの取得を省略できます．

`send()`，`recv()`，`sendto()`，`recvfrom()`の`flags`は0のみです．これらはPOSIX `errno`ではなく，負の`OS_*`値を直接返します．上記は互換性上の制限であり，ソケットの機能拡張時には本節を見直してください．

## ユーザプログラムの例

| コマンド | 役割 | ポート |
|---|---|---|
| `nc` | TCP/UDPクライアント・サーバのテスト | 指定値 |
| `httpd` | `/etc/index.html`を配信するHTTP/1.0サーバ | TCP 80 |
| `tcp_echod [PORT]` | 受信したバイト列を返すTCP Echoサーバ | TCP 7（既定） |
| `udp_echod [PORT]` | 受信したデータグラムを送信元へ返すUDP Echoサーバ | UDP 7（既定） |

PicoNIX側で`tcp_echod 1234`を起動し，Linuxから`nc 192.168.4.1 1234`で接続できます．UDPならPicoNIXで`udp_echod 1234`を起動し，Linuxから`printf 'hello\n' | nc -u -w 2 192.168.4.1 1234`を実行します．同一のポート7を使う場合もTCPとUDPは別プロトコルです．

`httpd`を起動すれば`http://192.168.4.1/`でページを表示できます．紹介ページは`edit /etc/index.html`で直接編集・保存できます．`/etc`の内容もRAM上にあるため，再起動時に元に戻ります．`lsof`ではソケットとローカルポートを確認できます．

## カーネル標準サービス

- TCP 23：Telnetによるシェル操作．
- UDP 2000：`READ`，`WRITE`，`ENABLE`，`DISABLE`による許可されたGPIO・ADC・PWM・フレームバッファ操作．

UDP 2000の例：`printf 'READ /dev/adc/temp\n' | nc -u -w 2 192.168.4.1 2000`．認証・暗号化はないため，信頼できる実験ネットワークに限定してください．

## 実装の見どころ

カーネルのネットワークサービスと，ソケットAPIを使う通常のユーザアプリケーションを区別しています．`trace`でシステムコールを，`lsof`でソケットのFDとポートを観察できます．開発の経緯を記録した`docs/en/NETWORK_STAGE*.md`などは履歴資料です．現在の仕様は本書を優先してください．

---

**関連リンク：** [開発ガイド一覧](../../README.md) ｜ [Part C 教科書](../../../learn/C/README.md) ｜ [English](../../../../en/development/apps/c/NETWORK-PROGRAMMING.md)
