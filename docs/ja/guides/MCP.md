# MCPサーバの利用（Pico W）

PicoNIXの`mcpd`は，Pico W上で動作する**実験用MCPサーバ**です．TCP 8080番の`POST /mcp`を受け付け，既存のHTTPサーバ`httpd`（TCP 80番）とは独立しています．

[English](../../en/guides/MCP.md)

> **安全上の注意：** 認証・TLSはありません．`--shell`を指定すると遠隔からコマンド実行やファイル書込みが可能になります．インターネットや共有LANに公開せず，隔離した実験用ネットワークでのみ使用してください．`Origin`ヘッダを持つ要求は拒否しますが，これは認証の代わりにはなりません．

## 1．起動とツール

Pico Wでネットワークを有効にした後，次のどちらかを実行します．

```sh
mcpd &
# コマンド実行を含む実験だけで使用する場合
mcpd --shell &
```

| 起動方法 | MCPに公開するツール | 用途 |
|---|---|---|
| `mcpd &` | `get_temperature` | 内蔵温度センサの読み取り |
| `mcpd --shell &` | 上記に加えて`get_manual`，`execute_command`，`write_file` | マニュアル参照と遠隔操作 |

`get_manual()`は`/man/README`の目次を返し，`topic`を指定すると個別ページを返します．`execute_command`は短い**非対話**コマンド専用です．`write_file`は`/tmp/NAME`のASCIIテキストを上書きします（最大2,048バイト，要求本文の制限にも依存）．通常起動では書込み・コマンド実行は公開されません．

`mcpd`は自動起動されません．サーバ利用中も`httpd`を停止する必要はありません．

## 2．Linux PCから確認する（LLM不要）

リポジトリのルートディレクトリで実行します．接続先はPico WのIPアドレスに置き換えてください．以下はAPモードの例です．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3"

# ツール一覧を表示する（呼び出しはしない）
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none

# 通常モードでも実行できる読み取りツール
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_temperature
```

`mcpd --shell &`で起動している場合は，`--tool get_manual`で目次を取得できます．`--tool all`は安全側の設計で，公開中の**`get_temperature`と`get_manual`だけ**を呼び出します．`execute_command`や`write_file`を自動的には実行しません．通信モードを明示したい場合は`--mode 2026-07-28`を指定できます．

## 3．手動HTTPテスト

Python SDKを介さず，HTTP要求を確認したい場合の例です．

```sh
PICO_IP=192.168.4.1
curl -i -X POST "http://$PICO_IP:8080/mcp" \
  -H 'Content-Type: application/json' \
  -H 'Accept: application/json, text/event-stream' \
  -H 'MCP-Protocol-Version: 2026-07-28' \
  -H 'Mcp-Method: server/discover' \
  --data '{"jsonrpc":"2.0","id":1,"method":"server/discover","params":{"_meta":{"io.modelcontextprotocol/protocolVersion":"2026-07-28","io.modelcontextprotocol/clientInfo":{"name":"curl-test","version":"1.0"},"io.modelcontextprotocol/clientCapabilities":{}}}}'
```

要求ではHTTPの`Mcp-Method`とJSONの`method`を一致させます．`tools/call`では`Mcp-Name`と`params.name`も一致させる必要があります．詳細は「実装と拡張」章を参照してください．

## 4．現行実装の制約

- MCP `2026-07-28`の一部に対応し，`server/discover`，`tools/list`，`tools/call`を実装しています．すべてのMCPクライアントとの互換性を保証するものではありません．
- HTTP要求バッファは3,600バイト，JSON本文は最大3,200バイトで，接続は1件ずつ処理します．
- `execute_command`のコマンド文字列はASCIIで最大119バイト，出力は最大4 KiBです．応答はHTTP/1.1のchunked転送を使用します．
- SSE，旧形式の`initialize`，通知，複数クライアントの同時処理には対応していません．
- `vi`，`edit`，`fd`，`mem`，`watch`，`pong`などの対話型・全画面プログラムを`execute_command`から起動しないでください．



## 5．ローカルLLMとエージェントの利用

### 全体構成

```text
利用者 → host/mcp_llm_client.py ←→ OpenAI互換LLMサーバ
                  ↓ MCP（TCP 8080）
             Pico Wのmcpd
                  ↓
          /dev・/proc・/man・シェル
```

LLMがFunction Callingでツールを選び，ホストクライアントがMCPに変換します．ツール定義はMCPの`tools/list`から自動取得されます．

### Pico Wでサーバを起動

隔離した実験ネットワークで実行します．

```sh
mcpd --shell &
```

`--shell`では`get_temperature`，`get_manual`，`execute_command`，`write_file`が公開されます．**認証・TLSがなく，LLM経由のコマンドがPico W上で実行されます．** ホストクライアントはツール実行前の確認を自動では求めません．重要な操作はLLMに任せず，必要に応じてクライアント側へ承認機構を実装してください．一般LANやインターネットには公開しないでください．

### Linux側で接続

リポジトリのルートで実行します．`192.168.4.1`はPico WのAPモードでの接続例です．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3" openai

# 先にMCPサーバとの通信を確認
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none
```

`host/mcp_llm_client.py`冒頭の設定値を，実際の環境に合わせます．`######`はプレースホルダです．

| 設定 | 内容 |
|---|---|
| `LLM_URL` | OpenAI互換APIのベースURL（通常`/v1`を含む） |
| `LLM_MODEL` | 使用するモデル名（Function Calling対応が必要） |
| `LLM_API_KEY` | LLMサーバ用のAPIキー |
| `MCP_URL` | Pico Wの`http://IP:8080/mcp` |
| `LLM_REASONING_EFFORT` | 思考量の指定．有効な値はサーバ／モデル依存 |
| `SHOW_THINKING` | 思考テキストが提供される場合の表示切替 |

```sh
python host/mcp_llm_client.py
```

`exit`か`quit`で終了します．LLMの回答はストリーミング表示されます．`SHOW_THINKING=False`でも，サーバから思考データが届く場合に簡単な進行表示が出ます．MCP応答のchunked転送とは別の仕組みです．

### 操作例

次は入力例です．**モデルが実際に選ぶツールや引数は保証されません．**

| 利用者の入力 | 想定するMCPツール |
|---|---|
| 「現在の温度を調べて」 | `get_temperature` |
| 「IPアドレスを調べて」 | `execute_command`で`ip` |
| 「実行中のプロセスを一覧にして」 | `execute_command`で`ps` |
| 「LEDにハートを表示して」 | `get_manual`で表示方法を参照後，`execute_command`で`echo heart > /dev/fbchar` |

`get_manual()`は`/man/README`の目次を返します．例えば`get_manual({"topic":"languages/c"})`で対象ページを取得します．廃止された`get_wifi_info`や`display_message`は公開されません．

`execute_command`は短い非対話コマンドに限ります．`vi`，`edit`，`fd`，`mem`，`watch`，`pong`などの全画面プログラムを実行しないでください．`write_file`は`/tmp/NAME`のASCIIテキストを上書きするため，既存ファイルへの影響に注意してください．

### うまく動かない場合

- **接続できない：** Pico Wで`mcpd --shell &`が動作しているか，`MCP_URL`のIPとポートが正しいか，まず`host/mcp_client.py --tool none`で確認します．
- **ツールが少ない：** `mcpd &`の通常モードは温度取得だけです．必要に応じて`--shell`モードで起動し直します．
- **LLMがツールを呼ばない：** LLMのChat Completions APIとFunction Calling対応を確認します．モデルによっては適切にツールを選べません．
- **`reasoning_effort`でエラー：** 対象サーバが指定値に対応しているか確認してください．

`host/mcp_client.py`による直接接続と，`host/mcp_llm_client.py`によるLLM連携を切り分けて確認することが重要です．


### LLMエージェントの運用方針

PicoNIXはLinuxではありません．`bash`，`sudo`，`apt`，`gcc`，`ssh`や完全なPOSIXシェルが存在するとは仮定しないでください．最初に`get_manual()`で索引を調べ，必要な`topic`だけを取得するか，`execute_command`で`help`を確認します．

- 観察には`ps`，`ip`，`cat /proc/net/wifi`，`cat /dev/adc/temp`などの短いコマンドを使用します．対話入力や全画面表示が必要なコマンドは避けてください．
- `execute_command`は呼出しごとに独立した非対話シェルを起動します．前回の`cd`などの状態は引き継がれません．コマンドは印字可能なASCIIで最大119バイト，出力は最大4,096バイト，タイムアウトはおおむね3秒です．応答の完了表示が個別のコマンドの正確な終了コードを保証するわけではありません．
- `write_file`は`/tmp/NAME`のASCIIファイルを作成または上書きします．ファイル内容は最大2,048バイトで，JSON要求本文にも3,200バイトの上限があります．追記・分割書込みはできません．
- `execute_command`はサンドボックスではありません．ファイルの上書き，プロセスの終了，GPIOやネットワーク状態の変更は，実行前に利用者が内容を確認して承認する運用にしてください．**標準の`host/mcp_llm_client.py`には，ツール実行前の確認機構がありません．** System Promptだけを安全装置とみなさず，必要であればホスト側で承認処理を実装してください．
- ホスト側で実行コマンド，結果，エラー，出力の切り詰めを記録してください．接続断やタイムアウト時に，変更操作を無条件で再試行しないでください．

モデルに与える指示の一例です．

> PicoNIXはLinuxではなく，RAMとプロセス数に制限のある教育用OSです．不明なコマンドやオプションは推測せず，`get_manual`または`help`を参照してください．状態を変更する操作は，必ず利用者の承認を得てから実行してください．対話型や終了しないコマンドは実行せず，実際のツール出力と推測を区別してください．

### Open WebUIなどの外部エージェントで利用する場合

`host/mcp_llm_client.py`を使わず，Open WebUIなどのMCP対応ホストから接続することも考えられます．ただし，PicoNIXの`mcpd`はMCPの限定的な実装なので，**特定のバージョンのOpen WebUIから直接接続できることは保証していません**．まず前述の`host/mcp_client.py`でサーバとの通信を確認してください．

外部ホストに登録するときは，接続先として`http://PICO_IP:8080/mcp`を使用し，MCPツールとして登録します（OpenAPIツールではありません）．管理画面の名称や接続方式はバージョンによって変わるため，使用中のホストの説明を確認してください．また，ツール権限とモデルからの呼出し条件を確認し，読み取り専用の`get_temperature`などで接続を検証します．無認証の`mcpd --shell`を公開する場合は，利用者とネットワークの双方を厳しく制限してください．

KnowledgeなどへOSの説明を登録する場合も，内容が毎回自動的にモデルへ渡るとは限りません．重要な制限は実行ホスト側の承認処理でも守る必要があります．

## 6．`mcpd`の実装・拡張実習

以下は`userland/apps/mcpd.c`を読みながら進める開発実習です．既存のTCP・HTTP・JSON-RPC・MCP・ファイル入出力をつなぐ仕組みを扱います．

### 学習目標と構成

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

### 準備とソースの配置

Pico W，Linux PC，USB端末，Pico SDK・ARMツールチェーンを用意します．[セットアップ](../setup/SETUP.md)と[ネットワーク接続](../reference/network/NETWORK.md)を先に確認してください．普通のPicoにはWi-Fi通信機能がないため，この実習にはPico Wを使います．

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

現在の登録IDは`kernel/main.c`の`register_image()`で確認してください．以前の試作版のIDを固定の仕様として扱わないでください．ほかのアプリを増やした場合は，必ず現在の`register_image()`一覧で重複を調べてください．

### 段階A：TCPサーバ

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

### 段階B：HTTPを受け取る

`handle()`から`read_request()`を追ってください．この処理はTCPから受信したバイト列を`request[]`に蓄積し，`\r\n\r\n`でHTTPヘッダの終わりを探します．次に`Content-Length`を読み，指定されたバイト数の本文が届くまで待ちます．

- HTTP要求は`REQUEST_SIZE`に定めた固定長バッファ（現在の値はソースで確認）に蓄積します．
- JSON本文は`BODY_LIMIT`で上限を検査します．
- 固定応答には静的領域を使い，大きなシェル出力やROMマニュアルはチャンク転送します．
- 大きなバッファをスタックではなく静的領域に置き，4 KiBのスタック使用を抑えています．

受信FDは非ブロッキングです．`read_request()`では`select()`に`struct timeval {0, 5000}`を渡し，最大5ミリ秒ずつ読み取り準備を待ちます．`recv()`の結果が`OS_EAGAIN`なら再試行します．この実装では再試行回数に上限を設けています．

この段階で理解すべきことは，**TCPの1回の`recv()`とHTTPリクエスト1件が一致するとは限らない**ことです．ヘッダやJSON本文が分割して到着する場合に対応しています．一方，HTTPの全機能を実装したサーバではありません．

**確認課題：** `Content-Length`が欠けている場合，大きすぎる場合，`POST /mcp`以外を受け取った場合の分岐を調べてください．

### 段階C：JSON-RPCを解析する

`process_json()`，`member()`，`json_end()`，`rpc_result()`，`rpc_error()`を読んでください．基本形は次のとおりです．

```json
{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{}}
```

受信した`id`は，同じ要求への応答を対応付けるために使用します．`method`に応じて処理を分岐し，成功時には`result`，失敗時には`error`を返します．ただし，**実際の試作版ではこのJSONだけでは受け付けられません**．`MCP-Protocol-Version`や`Mcp-Method`などのHTTPヘッダ，`params._meta`のクライアント情報も必要です．送信例は「基本的な使い方」章を参照してください．

試作版のJSON解析は，固定バッファ，限定的な文字列処理，最大ネスト深さ8に制限されています．一般用途の完全なJSONパーサではありません．

**確認課題：** `id`がない要求，未知の`method`，HTTPヘッダの`Mcp-Method`と本文中の`method`が異なる要求について，応答の作り方を調べてください．

### 段階D：MCPのツールを公開する

現在の実装が扱う基本メソッドは次の3つです．

| メソッド | 機能 |
|---|---|
| `server/discover` | 対応バージョンなどを返す |
| `tools/list` | ツール名，説明，入力スキーマを返す |
| `tools/call` | 指定されたツールを呼び出し，結果を返す |

通常起動では`get_temperature`のみ公開します．`mcpd --shell`では`get_manual`，`execute_command`，`write_file`も公開します．

| ツール | 実際に読むファイル |
|---|---|
| `get_temperature` | `/dev/adc/temp` |
| `get_manual` | `/man/README`（目次），または`topic`で指定したROMページ（`--shell`のみ） |

`tools/call`は`params.name`を見て分岐します．温度取得ではデバイスを読み，`get_manual`では`topic`（省略時`README`）から`/man`配下のパスを求めてchunked応答で転送します．`execute_command`もchunked転送を利用します．この関数の本質は普通のUNIX系のファイル入出力です．

```c
int fd = open(path, O_RDONLY);
int n = read(fd, out, cap - 1);
close(fd);
```

*説明用の抜粋です．実装では`open()`と`read()`の失敗を検査し，読み取った結果をNUL終端しています．*

読み取った文字列は必要に応じてJSON文字列用にエスケープし，`content`配列内の`type: "text"`として返します．エラー時は`isError: true`を返します．`get_manual`はROMファイルを分割してHTTPチャンク転送します．ページ名は`topic`で指定でき，引数を省略すると目次を返します．

**確認課題：** 温度センサーの読取りとROMマニュアルの転送について，通常のJSON応答とchunked応答の違いを説明してください．

### 段階E：Pico Wでビルド・起動する

現行ソースに`mcpd`が登録されていることを確認してから，リポジトリ直下でPico W向けにビルドします．

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

### 段階F：Linux側から通信する

最初は「基本的な使い方」章の`curl`例でHTTPヘッダとJSON本文を直接確認すると，障害を切り分けやすくなります．次に公式Python SDK v2を利用するホスト側のサンプルを試します．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install 'mcp>=2,<3'
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool none
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_temperature
python host/mcp_client.py http://192.168.1.50:8080/mcp --tool get_manual
```

IPアドレスは実際のPico Wのものに変更します．`--tool none`は一覧取得だけを行います．このクライアントにLLMは含まれません．まずは**MCP通信の正しさをLLMと独立に検証**します．公式SDK v2とのツール列挙・実行は2026-11-06に実機確認済みです．接続で問題が起きた場合はエラーを記録し，HTTP手動テストへ戻って原因を切り分けてください．

### 段階G：ツールを1つ増やす（設計演習）

`tools/list`の公開情報と`tools/call`の分岐を対応付け，新しい**読み取り専用**ツールを設計してみましょう．例えば，短いテキストを返す`/proc`のファイルを候補にできます．ただし，そのファイルが実際に存在するか，読み取りに必要なFD数，返却データの最大長を先に確認します．

作業手順：

1. 公開する名前と説明，`inputSchema`を決める．
2. `tools/list`の返却JSONへ項目を追加する．
3. `tools/call`に対応する分岐を追加する．
4. デバイスの読み取り失敗，長いデータ，JSONエスケープを確認する．
5. ホスト側から一覧取得と呼び出しを再試験する．
6. `-Oz`ビルド後のPIC・RAM使用量を測る．

最初からGPIO書き込みなどの操作ツールを公開することは推奨しません．認証，入力値の検証，操作対象の制限を設計してから取り組んでください．

### 調査・評価と発展課題

| 観察したいこと | 手段 | 確認ポイント |
|---|---|---|
| タスクの存在 | `ps` | `mcpd`がバックグラウンドで動作するか |
| TCP接続とFD | `lsof` | 接続中・切断後のFDの変化 |
| システムコール | `trace` | `accept()`，`select()`，`recv()`，`open()`，`read()`の呼び出し |
| RAM配置 | `mem`，ビルドの配置検査 | PIC，BSS，スタックと24 KiBの関係 |
| エラー処理 | 不正HTTP要求や存在しないツール | エラー応答と接続終了の挙動 |

`trace`の対象指定など各コマンドの引数は[traceガイド](TRACE.md)，[memガイド](MEM.md)を参照してください．

**未実装・未検証：** SSE応答，旧`initialize`との互換，完全なJSON/HTTP解析，認証，TLS，同時接続処理，MCP全機能への準拠と長期安定性試験．この段階で「MCP完全準拠」と説明してはいけません．

#### まとめ

この実習の中心は，**新しいMCP専用システムコールを追加せずに，既存のUNIX風APIだけでネットワークサービスを作れること**です．`mcpd`の機能を増やす前に，まずコードを読み，HTTPのやり取りを確認し，限られたRAMに収まるかを検査してください．

### ローカルLLMとの連携（実機確認済み）

基本的なMCP通信を確認できたら，「LLMとの連携」章へ進んでください．`host/mcp_llm_client.py`はOpenAI互換のFunction CallingをMCPに橋渡しし，通常の質問ではツールを使いません．LLMサーバ設定は`######`で伏せています．


### Open WebUIにPicoNIX固有の説明を登録する

Open WebUIのKnowledge機能を使用する場合は，この`docs/ja/guides/MCP.md`を登録し，MCPサーバは`http://PICO_IP:8080/mcp`として別途接続します．知識の登録だけではツールを呼べません．また，Knowledgeの全文が必ず各推論に渡されるわけではありません．変更操作の実行確認はホスト側で実装してください．
