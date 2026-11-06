# MCPサーバ `mcpd` を作る — Cユーザアプリケーション開発実習

*Pico W向け・発展実習（試作版の読解と段階的な改造）*

この教材では，PicoNIXのユーザ空間で動くMCP（Model Context Protocol）サーバ`mcpd`を題材に，**TCP → HTTP → JSON-RPC → MCPツール → デバイスファイル**のつながりを学びます．Linux PCからアクセスする`host/mcp_client.py`も使用します．LLMは必要ありません．

**前提と検証状態：** この文書は`userland/apps/mcpd.c`の試作実装をもとにしています．2026-07-28形式の一部を対象とし，**公式MCP Python SDK v2とのツール列挙・実行，ローカルLLMからの呼び出しは2026-11-06にPico W実機で成功**しました．ただし，MCP全仕様への準拠，長期安定性，最新版の厳密なメモリ使用量は未検証です．各段階の動作例は目標・確認手順であり，成功を保証する記録ではありません．

**安全上の注意：** 試作版には認証とTLSがありません．`Origin`付き要求を拒否しますが，これだけでアクセス制御にはなりません．公開LANやインターネットには接続せず，隔離された実験用ネットワークでのみ利用してください．特に書き込みを行うツールは初期版に追加しないでください．

[English](../../../../en/development/apps/c/MCP-SERVER-DEVELOPMENT.md) ｜ [MCP操作ガイド](../../../guides/MCP.md) ｜ [Cプログラミング](PROGRAMMING.md) ｜ [ソケット開発](NETWORK-PROGRAMMING.md)

## 1．学習目標と構成

次の5点を説明できることを目標にします．

1. `socket()`，`bind()`，`listen()`，`accept()`によりTCPの待受を開始する仕組み．
2. TCPのバイト列からHTTPヘッダと本文を取り出す方法．
3. JSON-RPCの`method`と`id`を読み取り，応答へ反映する方法．
4. MCPの`tools/list`と`tools/call`をデバイスファイルの読み取りに接続する仕組み．
5. PIC，24 KiBユーザスロット，4 KiBスタックというPicoNIX固有の制約．

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

## 2．準備とソースの配置

Pico W，Linux PC，USB端末，Pico SDK・ARMツールチェーンを用意します．[セットアップ](../../../setup/SETUP.md)と[ネットワーク接続](../../../reference/network/NETWORK.md)を先に確認してください．普通のPicoにはWi-Fi通信機能がないため，この実習にはPico Wを使います．

主なソースと関連ファイルは次のとおりです．

| ファイル | 役割 |
|---|---|
| `userland/apps/mcpd.c` | TCP・HTTP・JSON-RPC・MCP処理とデバイス読み取り |
| `userland/lib/unistd.h` | ソケット，`select()`，`read()`などの宣言 |
| `userland/lib/fcntl.h` | `fcntl()`，`O_NONBLOCK`など |
| `CMakeLists.txt` | `mcpd`のビルド対象への追加 |
| `kernel/main.c` | `mcpd`イメージのコマンド登録 |
| `host/mcp_client.py` | Linux側のテスト用MCPクライアント |
| `docs/ja/guides/MCP.md` | 利用方法，対応範囲，手動HTTPテスト |

試作パッチでは，`mcpd`の登録IDは**49**です．ほかのアプリを増やした場合は，必ず現在の`register_image()`一覧で重複を調べてください．

## 3．段階A：TCPサーバ

まず`mcpd.c`末尾の`main()`を読みます．重要な呼び出し順は次のとおりです．

```c
int server = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in address = {0};
address.sin_family = AF_INET;
address.sin_port = htons(8080);

bind(server, (const struct sockaddr *)&address, sizeof address);
listen(server, 1);
int client = accept(server, 0, 0);
```

*これは呼び出し順を示す抜粋です．元のコードには戻り値の検査，ループ，`close()`があります．実際に利用する場合は必ず元のエラー処理を残してください．*

`accept()`は既定で接続待ちを行います．接続が成立すると，試作版では受理したFDを`fcntl(client, F_SETFL, O_NONBLOCK)`で非ブロッキングにし，`handle(client)`で1件処理して閉じます．**複数クライアントを同時には処理しません．**

**確認課題：** `socket()`や`bind()`が失敗した場合の戻り値処理と，受理したFDがどこで`close()`されるかをソースから探してください．

## 4．段階B：HTTPを受け取る

`handle()`から`read_request()`を追ってください．この処理はTCPから受信したバイト列を`request[]`に蓄積し，`\r\n\r\n`でHTTPヘッダの終わりを探します．次に`Content-Length`を読み，指定されたバイト数の本文が届くまで待ちます．

- `REQUEST_SIZE = 3072`：要求全体の固定バッファ．
- `BODY_LIMIT = 1536`：JSON本文の上限．
- `RESPONSE_SIZE = 1400`：JSON応答の固定バッファ．
- 大きなバッファをスタックではなく静的領域に置き，4 KiBのスタック使用を抑えています．

受信FDは非ブロッキングです．`read_request()`では`select()`に`struct timeval {0, 5000}`を渡し，最大5ミリ秒ずつ読み取り準備を待ちます．`recv()`の結果が`OS_EAGAIN`なら再試行します．この実装では再試行回数に上限を設けています．

この段階で理解すべきことは，**TCPの1回の`recv()`とHTTPリクエスト1件が一致するとは限らない**ことです．ヘッダやJSON本文が分割して到着する場合に対応しています．一方，HTTPの全機能を実装したサーバではありません．

**確認課題：** `Content-Length`が欠けている場合，大きすぎる場合，`POST /mcp`以外を受け取った場合の分岐を調べてください．

## 5．段階C：JSON-RPCを解析する

`process_json()`，`member()`，`json_end()`，`rpc_result()`，`rpc_error()`を読んでください．基本形は次のとおりです．

```json
{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}
```

受信した`id`は，同じ要求への応答を対応付けるために使用します．`method`に応じて処理を分岐し，成功時には`result`，失敗時には`error`を返します．ただし，**実際の試作版ではこのJSONだけでは受け付けられません**．`MCP-Protocol-Version`や`Mcp-Method`などのHTTPヘッダ，`params._meta`のクライアント情報も必要です．送信例は[MCP操作ガイド](../../../guides/MCP.md)を参照してください．

試作版のJSON解析は，固定バッファ，限定的な文字列処理，最大ネスト深さ8に制限されています．一般用途の完全なJSONパーサではありません．

**確認課題：** `id`がない要求，未知の`method`，HTTPヘッダの`Mcp-Method`と本文中の`method`が異なる要求について，応答の作り方を調べてください．

## 6．段階D：MCPのツールを公開する

現在の試作版が実装するメソッドは以下の3つです．

| メソッド | 機能 |
|---|---|
| `server/discover` | 対応バージョンなどを返す |
| `tools/list` | ツール名，説明，入力スキーマを返す |
| `tools/call` | 指定されたツールを呼び出し，結果を返す |

`tools/list`で公開するツールは次の2つです．

| ツール | 実際に読むファイル |
|---|---|
| `get_temperature` | `/dev/adc/temp` |
| `get_wifi_info` | `/proc/net/wifi` |

`tools/call`は`params.name`を見て対象パスを選び，`read_device()`を呼びます．この関数の本質は普通のUNIX系のファイル入出力です．

```c
int fd = open(path, O_RDONLY);
int n = read(fd, out, cap - 1);
close(fd);
```

*説明用の抜粋です．実装では`open()`と`read()`の失敗を検査し，読み取った結果をNUL終端しています．*

読み取った文字列は`append_escaped()`でJSON文字列用にエスケープし，`content`配列内の`type: "text"`として返します．エラー時は`isError: true`を返します．`get_wifi_info`は一度に最大255バイトを読み取るため，**出力が長い場合は途中で切れる**ことに注意してください．

**確認課題：** 2ツールに共通するコードと，ツールごとに異なる部分を分けて説明してください．

## 7．段階E：Pico Wでビルド・起動する

`mcpd`を含む試作パッチを適用し，既存のアプリ登録と重複しないことを確認してから，リポジトリ直下でPico W向けにビルドします．

```sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

ビルドでは`-Oz`によるPICイメージが24 KiBユーザスロット（うちスタック予約4 KiB）に収まることを確認してください．試作時には代替EABI関数を使用した暫定リンクによって収容見込みを確認しましたが，**これは正式ビルドの合格を意味しません**．

UF2を書き込んでWi-Fi接続を確立したら，PicoNIXの端末で次を試します．

```sh
ls /bin
mcpd &
ps
```

`mcpd`が`/bin`にない場合は，`CMakeLists.txt`のアプリ一覧，`kernel/main.c`の画像宣言・登録，書き込んだUF2の更新日時を調べます．TCP 8080番を他のプロセスが使用していないことも確認してください．

## 8．段階F：Linux側から通信する

最初は[MCP操作ガイド](../../../guides/MCP.md)の`curl`例でHTTPヘッダとJSON本文を直接確認すると，障害を切り分けやすくなります．次に公式Python SDK v2を利用するホスト側のサンプルを試します．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install 'mcp>=2,<3'
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool none
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_temperature
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_wifi_info
```

IPアドレスは実際のPico Wのものに変更します．`--tool none`は一覧取得だけを行います．このクライアントにLLMは含まれません．まずは**MCP通信の正しさをLLMと独立に検証**します．公式SDK v2とのツール列挙・実行は2026-11-06に実機確認済みです．接続で問題が起きた場合はエラーを記録し，HTTP手動テストへ戻って原因を切り分けてください．

## 9．段階G：ツールを1つ増やす（設計演習）

`tools/list`の公開情報と`tools/call`の分岐を対応付け，新しい**読み取り専用**ツールを設計してみましょう．例えば，短いテキストを返す`/proc`のファイルを候補にできます．ただし，そのファイルが実際に存在するか，読み取りに必要なFD数，返却データの最大長を先に確認します．

作業手順：

1. 公開する名前と説明，`inputSchema`を決める．
2. `tools/list`の返却JSONへ項目を追加する．
3. `tools/call`に対応する分岐を追加する．
4. デバイスの読み取り失敗，長いデータ，JSONエスケープを確認する．
5. ホスト側から一覧取得と呼び出しを再試験する．
6. `-Oz`ビルド後のPIC・RAM使用量を測る．

最初からGPIO書き込みなどの操作ツールを公開することは推奨しません．認証，入力値の検証，操作対象の制限を設計してから取り組んでください．

## 10．調査・評価と発展課題

| 観察したいこと | 手段 | 確認ポイント |
|---|---|---|
| タスクの存在 | `ps` | `mcpd`がバックグラウンドで動作するか |
| TCP接続とFD | `lsof` | 接続中・切断後のFDの変化 |
| システムコール | `trace` | `accept()`，`select()`，`recv()`，`open()`，`read()`の呼び出し |
| RAM配置 | `mem`，ビルドの配置検査 | PIC，BSS，スタックと24 KiBの関係 |
| エラー処理 | 不正HTTP要求や存在しないツール | エラー応答と接続終了の挙動 |

`trace`の対象指定など各コマンドの引数は[traceガイド](../../../guides/TRACE.md)，[memガイド](../../../guides/MEM.md)を参照してください．

**未実装・未検証：** SSE応答，旧`initialize`との互換，完全なJSON/HTTP解析，認証，TLS，同時接続処理，MCP全機能への準拠と長期安定性試験．この段階で「MCP完全準拠」と説明してはいけません．

### まとめ

この実習の中心は，**新しいMCP専用システムコールを追加せずに，既存のUNIX風APIだけでネットワークサービスを作れること**です．`mcpd`の機能を増やす前に，まずコードを読み，HTTPのやり取りを確認し，限られたRAMに収まるかを検査してください．

## 11．ローカルLLMとの連携（実機確認済み）

基本的なMCP通信を確認できたら，[ローカルLLMクライアントの実習](../../../guides/MCP-LLM-CLIENT.md)へ進んでください．`host/mcp_llm_client.py`はOpenAI互換のFunction CallingをMCPに橋渡しし，通常の質問ではツールを使いません．LLMサーバ設定は`######`で伏せています．
