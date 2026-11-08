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


## 6．実装の読解と拡張

`mcpd`のTCP・HTTP・JSON-RPC処理や開発演習は，[MCPサーバ開発実習](../development/apps/c/MCP-DEVELOPMENT.md)に分離しました．利用方法は本ガイド，内部実装は開発ガイドを参照してください．

### Open WebUIにPicoNIX固有の説明を登録する

Open WebUIのKnowledge機能を使用する場合は，この`docs/ja/guides/MCP.md`を登録し，MCPサーバは`http://PICO_IP:8080/mcp`として別途接続します．知識の登録だけではツールを呼べません．また，Knowledgeの全文が必ず各推論に渡されるわけではありません．変更操作の実行確認はホスト側で実装してください．
