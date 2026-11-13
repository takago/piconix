# `mcpd`の実装と拡張

この文書ではPico W上のユーザ空間MCPサーバの実装構造，関連API，拡張方法を説明します．操作方法は[MCPリファレンス](../../../reference/commands/MCP.md)を参照してください．

## `mcpd`の構造と拡張手順

`userland/apps/mcpd.c`がTCP・HTTP・JSON-RPC・MCP・ファイル入出力をどのようにつないでいるかを示します．

### 処理系の構成

```text
Linux PC                       Pico W / PicoNIX
host/mcp_client.py             userland/apps/mcpd.c
      |                                |
      +---- TCP 8080 / POST /mcp ----->+
                                       | HTTP受信・JSON解析
                                       | MCPメソッド分岐
                                       +-- /dev/adc/temp
                                       +-- /proc/net/wifi
      <------------ JSON応答 -----------+
```

`mcpd`はカーネルに組み込むサービスではありません．`httpd`（TCP 80番）と独立した**ユーザ空間のコマンド**であり，OSの既存ソケットAPIとファイルAPIを利用しています．

### 準備とソースの配置

Pico W，Linux PC，USB端末，Pico SDK・ARMツールチェーンを用意します．[セットアップ](../../../setup/SETUP.md)と[ネットワーク接続](../../../reference/network/NETWORK.md)を先に確認してください．普通のPicoにはWi-Fi通信機能がないため，ネットワーク通信にはPico Wが必要です．

主なソースと関連ファイルは次のとおりです．

| ファイル | 役割 |
|---|---|
| `userland/apps/mcpd.c` | TCP・HTTP・JSON-RPC・MCP処理とデバイス読み取り |
| `userland/lib/unistd.h` | ソケット，`select()`，`read()`などの宣言 |
| `userland/lib/fcntl.h` | `fcntl()`，`O_NONBLOCK`など |
| `CMakeLists.txt` | `mcpd`のビルド対象への追加 |
| `kernel/main.c` | `mcpd`イメージのコマンド登録 |
| `host/mcp_client.py` | Linux側のテスト用MCPクライアント |
| `docs/ja/reference/commands/MCP.md` | 利用方法，対応範囲，手動HTTPテスト |

現行の`kernel/main.c`では，標準アプリ`mcpd`を**登録ID49**で登録しています．`mcpd`を追加登録する必要はありません．独自アプリを追加する場合は未使用IDを確認してください．

### TCP接続の受け付け

`mcpd.c`末尾の`main()`は，概ね次の呼び出し順で接続を受け付けます．

```c
int server = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in address = {0};
address.sin_family = AF_INET;
address.sin_port = htons(8080);

bind(server, (const struct sockaddr *)&address, sizeof address);
listen(server, 1);
/* select()で待受けFDが読み取り可能になるまで待つ． */
fd_set rd = 0;
FD_SET(server, &rd);
select(server + 1, &rd, 0, 0, 0);
int client = accept(server, 0, 0);
```

*これは呼び出し順を示す抜粋です．元のコードには戻り値の検査，ループ，`close()`があります．実際に利用する場合は必ず元のエラー処理を残してください．*

`mcpd`は`select()`で接続待ちを行い，読み取り可能になってから`accept()`を呼びます．接続が成立すると，受理したFDを`fcntl(client, F_SETFL, O_NONBLOCK)`で非ブロッキングにし，`handle(client)`で1件処理して閉じます．**複数クライアントを同時には処理しません．**


### HTTP要求の受信

`handle()`から`read_request()`を追ってください．この処理はTCPから受信したバイト列を`request[]`に蓄積し，`\r\n\r\n`でHTTPヘッダの終わりを探します．次に`Content-Length`を読み，指定されたバイト数の本文が届くまで待ちます．

- HTTP要求は`REQUEST_SIZE`に定めた固定長バッファ（現在の値はソースで確認）に蓄積します．
- JSON本文は`BODY_LIMIT`で上限を検査します．
- 固定応答には静的領域を使い，大きなシェル出力やROMマニュアルはチャンク転送します．
- 大きなバッファをスタックではなく静的領域に置き，4 KiBのスタック使用を抑えています．

受信FDは非ブロッキングです．`read_request()`は`select()`で最大5ミリ秒ずつ読み取り準備を待ちます．送信可能待ちには2ミリ秒，`execute_command`の出力パイプ監視には10ミリ秒を用います．これらは1回当たりの待機上限であり，HTTP要求とコマンド実行の全体には別途試行回数と時間の制限があります．`recv()`の結果が`OS_EAGAIN`なら再試行します．この実装では再試行回数に上限を設けています．

重要な点は，**TCPの1回の`recv()`とHTTPリクエスト1件が一致するとは限らない**ことです．ヘッダやJSON本文が分割して到着する場合に対応しています．一方，HTTPの全機能を実装したサーバではありません．


### JSON-RPCの解析

`process_json()`，`member()`，`json_end()`，`rpc_result()`，`rpc_error()`が解析と応答を担当します．基本形は次のとおりです．

```json
{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}
```

受信した`id`は，同じ要求への応答を対応付けるために使用します．`method`に応じて処理を分岐し，成功時には`result`，失敗時には`error`を返します．ただし，**このJSONだけでは受け付けられません**．`MCP-Protocol-Version`や`Mcp-Method`などのHTTPヘッダ，`params._meta`のクライアント情報も必要です．実際の要求形式は[MCPリファレンス](../../../reference/commands/MCP.md)を参照してください．

JSON解析は，固定バッファ，限定的な文字列処理，最大ネスト深さ8に制限されています．一般用途の完全なJSONパーサではありません．


### MCPツールの公開

現在の実装が扱う基本メソッドは次の3つです．

| メソッド | 機能 |
|---|---|
| `server/discover` | 対応バージョンなどを返す |
| `tools/list` | ツール名，説明，入力スキーマを返す |
| `tools/call` | 指定されたツールを呼び出し，結果を返す |

`mcpd`は`get_temperature`，`get_manual`，`execute_command`，`write_file`と疑似端末ツールを常時公開します．認証・TLSはありません．

| ツール | 実際に読むファイル |
|---|---|
| `get_temperature` | `/dev/adc/temp` |
| `get_manual` | `/man/README`（目次），または`topic`で指定したROMページ |

`tools/call`は`params.name`を見て分岐します．温度取得ではデバイスを読み，`get_manual`では`topic`（省略時`README`）から`/man`配下のパスを求めてchunked応答で転送します．`execute_command`もchunked転送を利用します．この関数の本質は普通のUNIX系のファイル入出力です．

```c
int fd = open(path, O_RDONLY);
int n = read(fd, out, cap - 1);
close(fd);
```

*説明用の抜粋です．実装では`open()`と`read()`の失敗を検査し，読み取った結果をNUL終端しています．*

読み取った文字列は必要に応じてJSON文字列用にエスケープし，`content`配列内の`type: "text"`として返します．エラー時は`isError: true`を返します．`get_manual`はROMファイルを分割してHTTPチャンク転送します．ページ名は`topic`で指定でき，引数を省略すると目次を返します．


### Pico Wでのビルドと起動

現行ソースに`mcpd`が登録されていることを確認してから，リポジトリ直下でPico W向けにビルドします．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.3.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

ビルドでは`-Oz`によるPICイメージが24 KiBユーザスロット（うちスタック予約4 KiB）に収まることを確認してください．実際のビルド結果でFlashとRAMスロットの配置を確認してください．

UF2を書き込んでWi-Fi接続を確立したら，PicoNIXの端末で次を試します．

```sh
ls /bin
mcpd &
ps
```

`mcpd`が`/bin`にない場合は，`CMakeLists.txt`のアプリ一覧，`kernel/main.c`の画像宣言・登録，書き込んだUF2の更新日時を調べます．TCP 8080番を他のプロセスが使用していないことも確認してください．

### Linuxホストからの通信確認

[MCPリファレンス](../../../reference/commands/MCP.md)の`curl`例でHTTPヘッダとJSON本文を直接確認すると，障害を切り分けやすくなります．次に公式Python SDK v2を利用するホスト側のサンプルを試します．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install 'mcp>=2,<3'
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool none
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_temperature
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_manual
```

IPアドレスは実際のPico Wのものに変更します．`--tool none`は一覧取得だけを行います．このクライアントにLLMは含まれません．まずは**MCP通信の正しさをLLMと独立に検証**します．公式SDK v2との基本的なツール列挙・呼び出しは実機で確認されていますが，完全準拠や長期安定性を示すものではありません．接続で問題が起きた場合はエラーを記録し，HTTP手動テストへ戻って原因を切り分けてください．

### 読み取り専用ツールの追加手順

`tools/list`の公開情報と`tools/call`の分岐を対応付け，新しい**読み取り専用**ツールを追加する場合は，次の点を確認します．例えば，短いテキストを返す`/proc`のファイルを候補にできます．ただし，そのファイルが実際に存在するか，読み取りに必要なFD数，返却データの最大長を先に確認します．

作業手順：

1. 公開する名前と説明，`inputSchema`を決める．
2. `tools/list`の返却JSONへ項目を追加する．
3. `tools/call`に対応する分岐を追加する．
4. デバイスの読み取り失敗，長いデータ，JSONエスケープを確認する．
5. ホスト側から一覧取得と呼び出しを再試験する．
6. `-Oz`ビルド後のPIC・RAM使用量を測る．

最初からGPIO書き込みなどの操作ツールを公開することは推奨しません．認証，入力値の検証，操作対象の制限を設計してから取り組んでください．

### 動作確認と診断

| 観察したいこと | 手段 | 確認ポイント |
|---|---|---|
| タスクの存在 | `ps` | `mcpd`がバックグラウンドで動作するか |
| TCP接続とFD | `lsof` | 接続中・切断後のFDの変化 |
| システムコール | `trace` | `accept()`，`select()`，`recv()`，`open()`，`read()`の呼び出し |
| RAM配置 | `mem`，ビルドの配置検査 | PIC，BSS，スタックと24 KiBの関係 |
| エラー処理 | 不正HTTP要求や存在しないツール | エラー応答と接続終了の挙動 |

`trace`の対象指定など各コマンドの引数は[traceガイド](../../../reference/commands/TRACE.md)，[memガイド](../../../reference/commands/MEM.md)を参照してください．

**未実装・未検証：** SSE応答，`initialize`，完全なJSON/HTTP解析，認証，TLS，同時接続処理，MCP全機能への準拠と長期安定性試験．「MCP完全準拠」と説明してはいけません．

### 実装上の要点

この実装が示すのは，**新しいMCP専用システムコールを追加せずに，既存のUNIX風APIだけでネットワークサービスを作れること**です．`mcpd`の機能を増やす前に，まずコードを読み，HTTPのやり取りを確認し，限られたRAMに収まるかを検査してください．

### ローカルLLMとの接続

ローカルLLMの接続方法は[MCPリファレンス](../../../reference/commands/MCP.md)を参照してください．`host/mcp_llm_client.py`はOpenAI互換のFunction CallingをMCPに橋渡しし，通常の質問ではツールを使いません．LLMサーバ設定は`######`で伏せています．


### HTTP送信とTCP close

`send_chunk()`は768バイト以下のチャンクを「長さ行＋データ＋CRLF」にまとめて`send_all()`へ渡します．TCP送信バッファは小さく，多数の小さな`tcp_write()`が送信キューを消費するためです．`send_all()`は部分書込みと`OS_EAGAIN`／0バイトを処理しますが，約3秒間進まなければ失敗として記録します．最終の`0\r\n\r\n`も送信結果を確認します．

通信切断は`kernel/net_socket.c`の`net_socket_tcp_close_client()`に伝わり，`tcp_close()`が失敗した場合はTCPポーリング経由で再試行します．この遅延closeと，アプリが正常な応答を全て送れたかどうかは別の状態です．`/proc/net/memory`のTCP診断カウンタ，既存の`trace`，`mcpd`コンソール，ホストの`host/mcp_stress.py`を組み合わせて調べます．実機での長時間連続通信は未検証です．
