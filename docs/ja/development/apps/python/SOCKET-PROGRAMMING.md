# Tiny PythonによるTCP/UDPソケットプログラミング

*開発ガイド — Pico W専用*

[Pythonの基本](PYTHON.md) | [デバイス操作](DEVICE-PROGRAMMING.md) | [ネットワーク仕様](../../../reference/network/NETWORK.md) | [English](../../../../en/development/apps/python/SOCKET-PROGRAMMING.md)

PicoNIXのPythonはCPythonの`socket`モジュールを使いません．**整数のファイルディスクリプタ（FD）**を操作する独自のAPIです．通常のPicoにはWi-Fiソケットがないため，実習にはPico Wを使用します．通信は暗号化されないので，信頼できる実験用ネットワークで実施してください．

## 準備と確認

Pico WのWi-Fi接続を確認し，`cat /proc/net/wifi`でIPアドレスを調べます．APモードでは一般にPico W側が`192.168.4.1`です．本機で実行するスクリプトは`edit /tmp/demo.py`で入力し，`python /tmp/demo.py`で起動します．TCPの既定サービスとポートが競合しないようにしてください．

## API一覧

| Python API | 意味 |
|---|---|
| `socket(1)` / `socket(2)` | TCP / UDPのFDを生成 |
| `connect(fd, "127.0.0.1", 80)` | TCP接続開始．接続完了は別途確認 |
| `state = status(fd)` | TCP状態．0=接続処理中，1=確立，負値=失敗など |
| `listen(fd, 9000)` | TCP待受開始 |
| `client = accept(fd)` | TCP接続を受入れ．未到着は負値（通常-11） |
| `bind(fd, 9000)` | UDP受信ポートを設定 |
| `n = send(fd, data)` | TCP送信．0や部分送信に注意 |
| `data = recv(fd, 64)` | TCP受信．最大64バイト |
| `n = sendto(fd, data, "192.168.4.2", 9000)` | UDPデータグラム送信 |
| `data = recvfrom(fd, 64)` | UDPデータグラム受信 |
| `peerport()` / `sendto(fd, data, "peer", 0)` | 直前のUDP送信元ポート取得／送信元への返信 |
| `close(fd)` | FD解放 |

`connect()`と`sendto()`には数値IPv4アドレスを文字列リテラルで与えます．ホスト名解決用のPython APIはありません．`"127.0.0.1"`は同じPico W内部のループバックを表します．

### 戻り値を読むときの注意

PicoNIXのユーザソケットは`O_NONBLOCK`の設定に関係なく**ポーリング方式**です．C APIでは未到着を`OS_EAGAIN`として返しますが，Pythonの`data = recv(...)`と`data = recvfrom(...)`はこの値を**空のバイト列**へ変換します．したがって，`len(data) == 0`だけでは未到着・TCP EOF・ゼロ長UDPデータグラムを区別できません．TCPの接続状態は`status(fd)`で別途調べます．

`accept()`は接続未到着時に負値を返すため，`client >= 0`を確認します．`send()`は部分送信または一時的な送信不可になる可能性があります．1回の送受信は最大64バイトで，`send()`・`sendto()`の`data`にはリテラル，文字列変数，`bytes`/リスト変数，`str(整数式)`を渡せます．`recv()`と`recvfrom()`は**代入文**で使います．

## 1．TCPクライアント：同一Pico WのHTTPサーバ

最初にPico W上でユーザ空間の`httpd`を起動してください．シェルで`httpd &`と実行してから，`/tmp/http.py`を作成します．

```python
fd = socket(1)
rc = connect(fd, "127.0.0.1", 80)
if rc == 0:
    state = 0
    while state == 0:
        state = status(fd)
        msleep(10)
    if state == 1:
        send(fd, "GET / HTTP/1.0\r\n\r\n")
        msleep(100)
        data = recv(fd, 64)
        write(1, data)
close(fd)
```

`python /tmp/http.py`を実行すると，HTTP応答の**先頭最大64バイト**を表示します．通信タイミングによって空の応答が表示されることもあります．この最小例は完全なHTTPクライアントではなく，応答全体の受信や送信の再試行は行いません．`while`ループの実行回数にはインタプリタの上限があります．

## 2．TCPサーバ：温度センサーの配信

既存のサンプル[`temp_tcp.py`](../../../../../userland/examples/python/temp_tcp.py)では，TCP 9000番で接続を待ち，内蔵温度センサーを`open("/dev/adc/temp", 0)`，`read()`で取得して送信し，接続FDを閉じます．ソースを`/tmp/temp_tcp.py`に入力またはコピーして実行します．

```sh
python /tmp/temp_tcp.py
```

PCからは次のように確認します（`PICO_IP`は実際のIPアドレスへ置き換えます）．

```bash
nc PICO_IP 9000
```

`listen()`用FDは残し，各`accept()`が返したクライアントFDを個別に`close()`します．サンプルは短い応答を1回だけ送信する教育用の最小例で，部分送信の再試行は省略しています．

## 3．UDPサーバ：受信元へ返信

既存のサンプル[`temp_udp.py`](../../../../../userland/examples/python/temp_udp.py)はUDP 9000番で受信し，送信元へ温度を返します．`recvfrom()`で受信した直近の送信元を`"peer"`とポート`0`で指定します．

```python
fd = socket(2)
rc = bind(fd, 9000)
if rc == 0:
    while True:
        data = recvfrom(fd, 64)
        if len(data) > 0:
            sendto(fd, "OK\n", "peer", 0)
        msleep(20)
close(fd)
```

これを`/tmp/udp.py`に保存し`python /tmp/udp.py`で実行します．別のPCからは次を試します．

```bash
printf 'hello' | nc -u -w 2 PICO_IP 9000
```

期待する応答は`OK`です．UDPでは1件のデータグラムを保持する受信バッファを使います．受信要求が保持中のデータグラムより小さいと，カーネルはエラーを返してそのデータグラムを保持します．Pythonは最大64バイトしか受け取れないため，**64バイトを超えるUDP要求を送らない**でください．`"peer"`は最後に受信した1件のみを指し，別の受信が入ると更新されます．

## 4．教育用実装の制約

- Pythonの`while`は1ループ最大10,000回で終了します．常駐サーバとしては設計されていません．
- TCPには管理スロット数の上限，UDPにはシステム全体で1つのユーザソケットという制限があります．詳しくは[ネットワーク仕様](../../../reference/network/NETWORK.md)を参照してください．
- `recv()`と`recvfrom()`の空配列はエラーコードではありません．通信状態を別途観察してください．
- 長いメッセージの分割，TCP部分送信の再試行，切断検知，タイムアウト管理はアプリ側の責任です．
- `socket()`や`connect()`などの呼出しは標準Pythonの構文と互換ではありません．`import socket`も使えません．

**関連資料：** [Pythonの基本](PYTHON.md) ／ [C言語のネットワークAPI](../c/NETWORK-PROGRAMMING.md)
