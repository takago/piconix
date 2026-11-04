# ソケットAPIとネットワークプログラミング（実習付き）

このガイドでは，**自分のCプログラム**を作り，Pico WからLinux PCへUDPを送り，続いてTCPでHTTPサーバに接続します．`nc`は完成した通信の比較・障害切り分けに使いますが，プログラミングの代わりにはしません．普通のPicoにはWi-Fi機能がないので，この実習にはPico Wが必要です．

## 1．実習の準備と全体像

必要なもの：Pico W，USBコンソール，Linux PC，Pico SDKとARMツールチェーン，両者が接続できる隔離された実験用ネットワーク．[セットアップ](../../../setup/SETUP.md)と[ネットワーク接続](../../../reference/network/NETWORK.md)でAPモードまたはSTAモードを準備してください．TelnetやUDPデバイス制御に認証・暗号化はありません．公開ネットワークでは使用しないでください．

```text
Pico W上のudp_once        → UDP 12345 → Linux PCのudp_echo_host.py
Pico W上のtcp_get         → TCP 8000  → Linux PCのpython -m http.server
  userland/apps/*.c → socket API → userland/lib/syscalls.c → SVC
     → kernel/kernel.c → kernel/net_socket.c → lwIP → CYW43439
```

1. PicoNIXで`ip addr`と`net`を実行し，Pico Wが接続済みか確認します．APモードならPico Wが通常`192.168.4.1`，PCが通常`192.168.4.2`ですが，**必ずPC側の`ip addr`で実際のPCアドレスを確認**してください．
2. Pico Wで`ps`と`lsof`を確認します．必要なら`udp_echod`や`nc -u`など，**UDPソケットを占有するプログラム**を終了してください．PicoNIXのユーザUDPソケットは同時に1つという制約があります．
3. Linux上の`python3`とファイアウォールを確認します．PC側の受信ポート12345（UDP）と8000（TCP）へのアクセスが許可された実験環境を使用します．

## 2．PicoNIXのソケットAPIの特徴

| 操作 | TCP | UDP | 注意点 |
|---|---|---|---|
| `socket()` | `SOCK_STREAM` | `SOCK_DGRAM` | `AF_INET`のみ |
| `bind()` | 呼出時に待受開始 | 受信ポートを設定 | `INADDR_ANY`のみ |
| `listen()` | 互換用の簡易処理 | 不要 | この関数だけで待受成功を判定しない |
| `accept(fd,NULL,NULL)` | 接続を取り出す | 不要 | 未接続は`OS_EAGAIN` |
| `connect()` | 接続開始 | 不要 | TCPは`tcp_status()`で完了確認 |
| `send()` / `recv()` | バイト列の送受信 | — | 部分送受信・`OS_EAGAIN`あり |
| `sendto()` / `recvfrom()` | — | データグラム送受信 | 送信元を省略するなら`NULL`指定 |
| `close()` | FDを解放 | FDを解放 | 必ず終了時に呼ぶ |

PicoNIXのC APIは完全なPOSIX互換ではありません．`errno`ではなく**負の`OS_*`値を直接返します**．`send()`などの`flags`は0のみです．`sockaddr_in.sin_port`は`htons()`でネットワークバイト順に変換します．IPv4アドレスは数値指定でありDNS名前解決はありません．詳細は[Cライブラリ](LIBC.md)と`userland/lib/unistd.h`を確認してください．

## 3．新しいコマンドをファームウェアに組み込む

以下のサンプルソースはリポジトリ内の`docs/examples/`に同梱しています．Linux PCの**リポジトリ直下**で実行します．

```sh
cp docs/examples/udp_once.c userland/apps/udp_once.c
cp docs/examples/tcp_get.c userland/apps/tcp_get.c
```

`CMakeLists.txt`の`foreach(APP IN ITEMS ... udp_echod)`の**現在の一覧の末尾**へ，`udp_once tcp_get`を追加します（既存のアプリを削除しない）．次に`kernel/main.c`の他の`extern`宣言の近くへ以下を追加します．

```c
extern const unsigned char piconix_udp_once_image_start[], piconix_udp_once_image_end[];
extern const unsigned char piconix_tcp_get_image_start[], piconix_tcp_get_image_end[];
```

同じ`main()`内で，既存の`register_image()`に続けて登録します．現行ソースでは登録ID49・50が未使用ですが，**改造済みのリポジトリでは空き番号を必ず再確認**してください．

```c
register_image(49, "udp_once", piconix_udp_once_image_start, piconix_udp_once_image_end);
register_image(50, "tcp_get", piconix_tcp_get_image_start, piconix_tcp_get_image_end);
```

新しいコマンドを登録する仕組み，PIC，RAMスロットの意味は[Cアプリケーション開発](PROGRAMMING.md)の第3～4節を参照してください．`-Oz`で生成したユーザアプリが24 KiBスロットに収まることもビルド時に検査されます．

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

UF2を書き込んだ後，`ls /bin`で両コマンドが現れることを確認します．`/bin`にないならCMake・`extern`・`register_image()`の3か所，または古いUF2を使っていないかを確認してください．

## 4．UDPプログラミング：1通送って返事を受け取る

### 4.1 PC上のUDPエコーサーバ

Linux PC側で起動し，その端末を開いたままにします．`udp_once`から受け取ったバイト列を同じ送信元に送り返すだけです．

```sh
python3 docs/examples/udp_echo_host.py 12345
```

サーバの全ソース（`docs/examples/udp_echo_host.py`）は次のとおりです．

```python
"""One-client UDP echo server for the PicoNIX networking tutorial (host PC)."""
import socket
import sys

port = int(sys.argv[1]) if len(sys.argv) > 1 else 12345
if not 1 <= port <= 65535:
    raise SystemExit("port must be 1..65535")
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
    sock.bind(("0.0.0.0", port))
    print(f"listening UDP {port}", flush=True)
    while True:
        payload, address = sock.recvfrom(512)
        print(f"{address}: {payload!r}", flush=True)
        sock.sendto(payload, address)
```

### 4.2 Pico W側のUDPクライアントのソース

`docs/examples/udp_once.c`の**省略のないコード**です．`IPv4`引数の4バイトを`sin_addr.s_addr`に配置し，`sendto()`で送信した後，`recvfrom()`を最大約2秒ポーリングします．

```c
/* Pico W: send one UDP datagram to an echo server on the PC and read the reply. */
#include "unistd.h"
#include "stdio.h"
#include <stdint.h>

static int ipv4(const char *s, uint32_t *result) {
    unsigned char *p = (unsigned char *)result;
    for (int i = 0; i < 4; ++i) {
        unsigned value = 0, digits = 0;
        while (*s >= '0' && *s <= '9') {
            value = value * 10u + (unsigned)(*s++ - '0');
            if (value > 255u || ++digits > 3) return -1;
        }
        if (!digits || (i < 3 ? *s++ != '.' : *s != '\0')) return -1;
        p[i] = (unsigned char)value;
    }
    return 0;
}

static int port_number(const char *s) {
    unsigned n = 0;
    if (!*s) return -1;
    for (; *s; ++s) {
        if (*s < '0' || *s > '9') return -1;
        n = n * 10u + (unsigned)(*s - '0');
        if (n > 65535u) return -1;
    }
    return n ? (int)n : -1;
}

int main(int argc, char **argv) {
    uint32_t address = 0;
    int port = argc == 3 ? port_number(argv[2]) : -1;
    if (argc != 3 || port < 0 || ipv4(argv[1], &address) < 0) {
        puts("usage: udp_once IPv4 PORT");
        return 1;
    }
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { printf("socket: %d\n", fd); return 1; }
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_port = htons((uint16_t)port);
    peer.sin_addr.s_addr = address;
    static const char msg[] = "hello from PicoNIX";
    int sent = sendto(fd, msg, sizeof(msg)-1u, 0,
                      (const struct sockaddr *)&peer, sizeof(peer));
    if (sent != (int)sizeof(msg)-1) {
        printf("sendto: %d\n", sent); close(fd); return 1;
    }
    char reply[80];
    for (int i = 0; i < 200; ++i) {
        int n = recvfrom(fd, reply, sizeof(reply), 0, 0, 0);
        if (n >= 0) {
            printf("received %d bytes: ", n);
            if (n > 0) write(1, reply, (unsigned)n);
            puts(""); close(fd); return 0;
        }
        if (n != OS_EAGAIN) {
            printf("recvfrom: %d\n", n); close(fd); return 1;
        }
        msleep(10);
    }
    puts("timeout (check the PC echo server and its firewall)");
    close(fd);
    return 1;
}
```

PCのIPアドレスが`192.168.4.2`，ポートが12345の場合の**PicoNIX側**の実行例です．

```text
piconix# udp_once 192.168.4.2 12345
received 18 bytes: hello from PicoNIX
```

上の表示は**成功時の想定例**であり，実機で取得したログではありません．PC側のサーバには送信元IP・ポートが表示されます．`sendto()`の結果は送信バイト数，`recvfrom()`は受信バイト数です．`OS_EAGAIN`は異常ではなく「まだデータがない」ことなので，短時間`msleep()`して再試行します．

### 4.3 比較実験

- PC側のサーバを止めて`udp_once`を実行し，**UDPは送信成功でも返事が保証されない**ことを確認します．
- PCのファイアウォールで受信が遮断された場合とIPの間違いを比較します．
- `lsof`でUDPソケットの存在を調べます．通信中に`tcpdump`を使うとデータグラムの往復を観察できます．

## 5．TCPプログラミング：HTTP応答を受け取る

Linux PCで次を実行します（この端末は実習中開いたままにします）．

```sh
python3 -m http.server 8000 --bind 0.0.0.0
```

次は`docs/examples/tcp_get.c`の**完全なソース**です．`connect()`の戻り値0は接続開始を意味し，**接続完了ではありません**．`tcp_status()`で接続を待ってからHTTP/1.0要求を送り，`recv()`を繰り返します．部分送信も考慮します．

```c
/* Pico W: minimal TCP client for an HTTP server on the host PC. */
#include "unistd.h"
#include "stdio.h"
#include <stdint.h>

static int ipv4(const char *s, uint32_t *result) {
    unsigned char *p = (unsigned char *)result;
    for (int i = 0; i < 4; ++i) {
        unsigned value = 0, digits = 0;
        while (*s >= '0' && *s <= '9') {
            value = value * 10u + (unsigned)(*s++ - '0');
            if (value > 255u || ++digits > 3) return -1;
        }
        if (!digits || (i < 3 ? *s++ != '.' : *s != '\0')) return -1;
        p[i] = (unsigned char)value;
    }
    return 0;
}

static int port_number(const char *s) {
    unsigned n = 0;
    if (!*s) return -1;
    for (; *s; ++s) {
        if (*s < '0' || *s > '9') return -1;
        n = n * 10u + (unsigned)(*s - '0');
        if (n > 65535u) return -1;
    }
    return n ? (int)n : -1;
}

int main(int argc, char **argv) {
    uint32_t address = 0;
    int port = argc == 3 ? port_number(argv[2]) : -1;
    if (argc != 3 || port < 0 || ipv4(argv[1], &address) < 0) {
        puts("usage: tcp_get IPv4 PORT");
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { printf("socket: %d\n", fd); return 1; }
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_port = htons((uint16_t)port);
    peer.sin_addr.s_addr = address;
    int r = connect(fd, (const struct sockaddr *)&peer, sizeof(peer));
    if (r < 0) { printf("connect: %d\n", r); close(fd); return 1; }
    int status = 0;
    for (int i = 0; i < 300; ++i) {
        status = tcp_status(fd);
        if (status != 0) break;
        msleep(10);
    }
    if (status != 1) { printf("not connected: %d\n", status); close(fd); return 1; }
    static const char request[] = "GET / HTTP/1.0\r\nHost: piconix-lab\r\n\r\n";
    unsigned off = 0;
    while (off < sizeof(request)-1u) {
        r = send(fd, request + off, sizeof(request)-1u-off, 0);
        if (r == OS_EAGAIN || r == 0) { msleep(10); continue; }
        if (r < 0) { printf("send: %d\n", r); close(fd); return 1; }
        off += (unsigned)r;
    }
    shutdown(fd, SHUT_WR);
    char buf[128];
    int idle = 0;
    for (;;) {
        r = recv(fd, buf, sizeof(buf), 0);
        if (r == 0) break;
        if (r == OS_EAGAIN) {
            if (++idle > 300) { puts("timeout reading response"); close(fd); return 1; }
            msleep(10); continue;
        }
        if (r < 0) { printf("recv: %d\n", r); close(fd); return 1; }
        idle = 0;
        int n = r, used = 0;
        while (used < n) {
            int k = write(1, buf + used, (unsigned)(n-used));
            if (k <= 0) { puts("write failed"); close(fd); return 1; }
            used += k;
        }
    }
    close(fd);
    return 0;
}
```

PicoNIX側で実行します．

```text
piconix# tcp_get 192.168.4.2 8000
HTTP/1.0 200 OK
...
```

`HTTP/1.0 200 OK`はPC側HTTPサーバの応答例です．ディレクトリやPython版で結果は変わります．`tcp_status()`が1以外で終了したらWi-Fi，接続先アドレス，TCP 8000の待受を確認します．長時間待機する場合は`Ctrl+C`で停止してください．

## 6．観察することとよくある失敗

| 症状 | 最初に確かめること |
|---|---|
| `socket`失敗 | Pico W用のUF2か，ソケットやタスクスロットが空いているか |
| `sendto`失敗 | 宛先数値IPv4・ポート・UDPスロット競合 |
| UDP受信だけタイムアウト | PC側サーバ・ファイアウォール・返事の宛先 |
| TCPが接続できない | PCのTCP 8000待受・STA/AP接続・IPアドレス |
| `OS_EAGAIN`が続く | データ未到着．短く待って再確認する．無限busy-loopにしない |
| `cannot start program` | RAMスロット不足やイメージのサイズを`ps`・ビルドログで調べる |

**考えてみよう：** (1) TCPとUDPで「送信の単位」はどう違うか．(2) TCPの`send()`が要求サイズより小さい値を返したら，なぜ再送処理が必要か．(3) カーネルに常駐するTelnetと，今作ったユーザプログラムはどこで動作するか．

既存の通信確認には[`nc`](../../../reference/network/NC.md)を使えますが，実装理解には`userland/apps/tcp_echod.c`・`udp_echod.c`・`httpd.c`を読み比べてください．カーネルのlwIP処理を拡張する場合は[ネットワーク実装ガイド](../../network/NETWORK-DEVELOPMENT.md)に進みます．

---

[開発ガイド一覧](../../README.md) ｜ [ネットワークリファレンス](../../../reference/network/README.md) ｜ [English](../../../../en/development/apps/c/NETWORK-PROGRAMMING.md)
