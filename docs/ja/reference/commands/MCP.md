# MCPサーバの利用（Pico W）

PicoNIXの`mcpd`は，Pico W上で動作する**実験用MCPサーバ**です．TCP 8080番の`POST /mcp`を受け付け，既存のHTTPサーバ`httpd`（TCP 80番）とは独立しています．

[English](../../../en/reference/commands/MCP.md)

> **安全上の注意：** 認証・TLSはありません．起動すると遠隔からコマンド実行やファイル書込みが可能になります．インターネットや共有LANに公開せず，隔離した実験用ネットワークでのみ使用してください．`Origin`ヘッダを持つ要求は拒否しますが，これは認証の代わりにはなりません．

## 1．起動とツール

Pico Wでネットワークを有効にした後，次を実行します．

```sh
mcpd &
```

`mcpd`は常に6つのツール（`get_temperature`，`get_manual`，`write_file`，`execute_command`，`terminal_send`，`terminal_read`）を公開します．**認証やTLSはないため，隔離した実験用ネットワークでのみ使用してください．**

`get_manual({})`または`get_manual({"topic":"index"})`は`/man/README`の目次を返します．目次で確認したトピック名を`topic`に指定すると個別ページを返します．`execute_command`は短い**非対話**コマンド専用です．`write_file`は`/tmp/NAME`のASCIIテキストを上書きします（最大2,048バイト，要求本文の制限にも依存）．書込み・コマンド実行は常時有効です．

`mcpd`は自動起動されません．サーバ利用中も`httpd`を停止する必要はありません．

## 2．Linux PCから確認する（LLM不要）

リポジトリのルートディレクトリで実行します．接続先はPico WのIPアドレスに置き換えてください．以下はAPモードの例です．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3"

# ツール一覧を表示する（呼び出しはしない）
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none

# 温度を取得する読み取りツール
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_temperature
```

`mcpd &`で起動している場合は，`--tool get_manual`で目次を取得できます．`--tool all`は安全側の設計で，公開中の**`get_temperature`と`get_manual`だけ**を呼び出します．`execute_command`や`write_file`を自動的には実行しません．通信モードを明示したい場合は`--mode 2026-07-28`を指定できます．

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
- HTTP要求バッファは2,800バイト，JSON本文は最大2,400バイトで，接続は1件ずつ処理します．
- `execute_command`のコマンド文字列はASCIIで最大119バイト，出力は最大4 KiBです．応答はHTTP/1.1のchunked転送を使用します．
- SSE，`initialize`，通知，複数クライアントの同時処理には対応していません．
- `vi`，`edit`，`fd`，`mem`，`watch`，`pong`，`reversi`，`tictactoe`，`maze`などの対話型・全画面プログラムを`execute_command`から起動しないでください．


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
mcpd &
```

`mcpd`では`get_temperature`，`get_manual`，`execute_command`，`write_file`，`terminal_send`，`terminal_read`の6ツールが公開されます．**認証・TLSがなく，LLM経由のコマンドがPico W上で実行されます．** ホストクライアントはツール実行前の確認を自動では求めません．重要な操作はLLMに任せず，必要に応じてクライアント側へ承認機構を実装してください．一般LANやインターネットには公開しないでください．

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

#### JSONL操作ログとトークン使用量

実行時に`logs/mcp_llm_YYYYMMDD-HHMMSS-ffffff.jsonl`を新規作成し，会話とMCP操作を逐次記録します．保存先を変える場合は`python host/mcp_llm_client.py --log /path/to/session.jsonl`とします．既存ファイルは上書きしません．終了後もログは残り，`logs/`はGit管理対象から除外されます．

ログには時刻，ユーザ入力，LLM回答，MCPのツール名・生引数・結果・エラー，API単位の`usage`，ターン集計，セッション集計をJSONL形式（1行1イベント）で保存します．`[USAGE]`はAPI呼び出しごと，`[TURN]`はユーザ入力1件に伴う全LLM呼び出しの合計，`[SESSION]`は起動中の全ターンの合計です．Promptには再送信された過去の会話も含まれるため，累積数値は会話本文の長さではありません．

使用量はOpenAI互換APIのストリーミング最終チャンクにある`usage.prompt_tokens`／`completion_tokens`を取得します．サーバが未対応なら`N/A`と表示し，計測されなかったAPI呼び出し件数を記録します．欠損がある合計値は部分集計であり，推測で補いません．モデルが思考本文を別フィールドで返しても，従来どおり次のAPI要求の会話履歴には加えず，JSONLにも思考本文は保存しません．ただしサーバが`completion_tokens`に思考トークンを含める場合はその数も合計に含まれます．

**注意：** JSONLには利用者の入力，実行コマンド，ファイル内容，端末出力などがそのまま保存されます．ログは所有者のみ読み書き可能な権限（0600）で新規作成しますが，共有・公開しないでください．JSONLへの保存だけで計画がLLMに引き継がれるわけではなく，現行の会話履歴管理は変更していません．


### 操作例

次は入力例です．**モデルが実際に選ぶツールや引数は保証されません．**

| 利用者の入力 | 想定するMCPツール |
|---|---|
| 「現在の温度を調べて」 | `get_temperature` |
| 「IPアドレスを調べて」 | `execute_command`で`ip` |
| 「実行中のプロセスを一覧にして」 | `execute_command`で`ps` |
| 「LEDにハートを表示して」 | `get_manual`で表示方法を参照後，`execute_command`で`echo heart > /dev/fbchar` |

`get_manual()`は`/man/README`の目次を返します．例えば`get_manual({"topic":"languages/c"})`で対象ページを取得します．

`execute_command`は短い非対話コマンドに限ります．`vi`，`edit`，`fd`，`mem`，`watch`，`pong`，`reversi`，`tictactoe`，`maze`などの全画面プログラムを実行しないでください．`write_file`は`/tmp/NAME`のASCIIテキストを上書きするため，既存ファイルへの影響に注意してください．

### うまく動かない場合

- **接続できない：** Pico Wで`mcpd &`が動作しているか，`MCP_URL`のIPとポートが正しいか，まず`host/mcp_client.py --tool none`で確認します．
- **ツールが少ない：** 最新の`mcpd`を再ビルドして起動し，`tools/list`を確認します．
- **LLMがツールを呼ばない：** LLMのChat Completions APIとFunction Calling対応を確認します．モデルによっては適切にツールを選べません．
- **`reasoning_effort`でエラー：** 対象サーバが指定値に対応しているか確認してください．

`host/mcp_client.py`による直接接続と，`host/mcp_llm_client.py`によるLLM連携を切り分けて確認することが重要です．


### LLMエージェントの運用方針

PicoNIXはLinuxではありません．`bash`，`sudo`，`apt`，`gcc`，`ssh`や完全なPOSIXシェルが存在するとは仮定しないでください．最初に`get_manual()`で索引を調べ，必要な`topic`だけを取得するか，`execute_command`で`help`を確認します．

- 観察には`ps`，`ip`，`cat /proc/net/wifi`，`cat /dev/adc/temp`などの短いコマンドを使用します．対話入力や全画面表示が必要なコマンドは避けてください．
- `execute_command`は呼出しごとに独立した非対話シェルを起動します．前回の`cd`などの状態は引き継がれません．コマンドは印字可能なASCIIで最大119バイト，出力は最大4,096バイト，タイムアウトはおおむね3秒です．`sh -c`は最後のコマンド（パイプラインでは最終段）の終了コードを返し，0以外ならMCPの`isError:true`と終了コードの表示で通知します．ただし，通信完了とコマンドの成功は別です．
- `write_file`は`/tmp/NAME`のASCIIファイルを作成または上書きします．ファイル内容は最大2,048バイトで，JSON要求本文にも2,400バイトの上限があります．追記・分割書込みはできません．
- `execute_command`はサンドボックスではありません．ファイルの上書き，プロセスの終了，GPIOやネットワーク状態の変更は，実行前に利用者が内容を確認して承認する運用にしてください．**標準の`host/mcp_llm_client.py`には，ツール実行前の確認機構がありません．** System Promptだけを安全装置とみなさず，必要であればホスト側で承認処理を実装してください．
- ホスト側で実行コマンド，結果，エラー，出力の切り詰めを記録してください．接続断やタイムアウト時に，変更操作を無条件で再試行しないでください．

モデルに与える指示の一例です．

> PicoNIXはLinuxではなく，RAMとプロセス数に制限のある教育用OSです．不明なコマンドやオプションは推測せず，`get_manual`または`help`を参照してください．状態を変更する操作は，必ず利用者の承認を得てから実行してください．対話型や終了しないコマンドは実行せず，実際のツール出力と推測を区別してください．

### Open WebUIなどの外部エージェントで利用する場合

`host/mcp_llm_client.py`を使わず，Open WebUIなどのMCP対応ホストから接続することも考えられます．ただし，PicoNIXの`mcpd`はMCPの限定的な実装なので，**特定のバージョンのOpen WebUIから直接接続できることは保証していません**．まず前述の`host/mcp_client.py`でサーバとの通信を確認してください．

外部ホストに登録するときは，接続先として`http://PICO_IP:8080/mcp`を使用し，MCPツールとして登録します（OpenAPIツールではありません）．管理画面の名称や接続方式はバージョンによって変わるため，使用中のホストの説明を確認してください．また，ツール権限とモデルからの呼出し条件を確認し，読み取り専用の`get_temperature`などで接続を検証します．無認証の`mcpd`を公開する場合は，利用者とネットワークの双方を厳しく制限してください．

KnowledgeなどへOSの説明を登録する場合も，内容が毎回自動的にモデルへ渡るとは限りません．重要な制限は実行ホスト側の承認処理でも守る必要があります．


## 6．実装の読解と拡張

`mcpd`のTCP・HTTP・JSON-RPC処理や実装例は，[MCPサーバ開発資料](../../development/apps/c/MCP-DEVELOPMENT.md)に分離しました．利用方法は本ガイド，内部実装は開発ガイドを参照してください．

### Open WebUIにPicoNIX固有の説明を登録する

Open WebUIのKnowledge機能を使用する場合は，この`docs/ja/reference/commands/MCP.md`を登録し，MCPサーバは`http://PICO_IP:8080/mcp`として別途接続します．知識の登録だけではツールを呼べません．また，Knowledgeの全文が必ず各推論に渡されるわけではありません．変更操作の実行確認はホスト側で実装してください．

## PTYによる対話型アプリの操作

LLMは`get_manual({"topic":"terminal-operations"})`で，入力前の画面確認，`text`と`key`の使い分け，シェルの復旧，通信障害時の再送防止を確認できます．

`mcpd`は起動時にPTYを1つ確保して対話シェル`sh`を起動します．`terminal_open`と`terminal_close`はありません．LLMは`terminal_read`で80×24画面を確認し，`terminal_send`の`text`でシェルコマンドを入力して`ENTER`で実行します．対話型アプリケーションはこのシェルから起動します．`terminal_send`は受理バイト数のみを返し，描画完了を待ちません．画面は`terminal_read`で別途取得します．画面専用PTYモードのため，通常のPTY出力リングは使用しません．

```text
terminal_read({})
terminal_send({"text":"fd /tmp"})
terminal_send({"key":"ENTER"})
terminal_read({})
terminal_send({"key":"DOWN"})
terminal_read({})
terminal_send({"text":"q"})
terminal_read({})
```

`fd`などの終了後は同じPTYのシェルへ戻ります．`exit`などでシェルそのものが終了した場合，次の端末要求でシェルを再生成します．再生成を検出した`terminal_send`は入力を送らずエラーを返すので，`terminal_read`で状態を確認してから操作してください．PTYとシェルは`mcpd`の終了時に解放します．起動時にシェル用のタスクスロットを1つ消費し，確保・起動できなければ`mcpd`も起動しません．

文字入力は`terminal_send({"text":"hello"})`で送れます．空白は`terminal_send({"text":" "})`または`terminal_send({"key":"SPACE"})`（短縮名`SP`）で送れます．キー名は`UP`，`DOWN`，`LEFT`，`RIGHT`，`ENTER`，`ESC`，`ESCAPE`，`TAB`，`SPACE`，`SP`，`BACKSPACE`，`BS`，`DELETE`，`CTRL_A`〜`CTRL_Z`です．入力はPTYの行規律を通り，`CTRL_C`は既存の前景タスク強制終了動作を使用します．スクロールバックはありません．

通常コマンドには引き続き`execute_command`を優先してください．MCPのPTYセッションは1つですが，カーネルは全体で最大2組のPTYを利用でき，6タスク上限を共有します．TTY内のコマンド起動は通常のPicoNIXシェル構文に従います．管理対象の`mcpd`をTTYから再起動しないでください．MCP経由で実行した子孫から`mcpd`またはその起動元のシェルを`kill()`する操作はカーネル側で拒否します．これは誤操作対策であり，完全なサンドボックスではありません．認証やTLSも未実装です．

MCPツールの`terminal_read`の説明には，SGR 4/24がカーソル位置，SGR 7/0が反転表示を表すことを明記しています．

`.screen`はPicoNIX独自の端末表示補助です．反転セルはSGR 7/0，端末カーソルの1文字はSGR 4/24で表します（下線属性はカーソル位置を示すだけで，文字セルへ保存しません．`ESC[?25l`で非表示中はマーカーを出さず，`ESC[?25h`で復帰します）．完全なANSI互換性や日本語の全角文字幅などは保証しません．詳しくは[PTYデバイス](../devices/PTY.md)を参照してください．

### PTY画面の実機確認

このサンプルは共有端末のシェルが入力待ちのときに実行してください．実行中の別アプリの画面に文字を送る可能性があります．

```sh
python3 host/mcp_pty_test.py http://192.168.4.1:8080/mcp --command fd --keys DOWN,UP,q
```

`terminal_read`は毎回80×24の現在画面を返します．ハッシュによる`unchanged`判定は行いません．画面は1回だけ読み出しますが，描画と並行した場合に原子的なスナップショットになる保証はありません．


### MCP通信の連続試験

`mcpd`はHTTP接続を1件ずつ処理する実験的なサーバであり，接続の同時受理を保証しません．
`Connection: close`のため，連続リクエストはTCP接続の確立・終了を繰り返します．
部分的なHTTP応答を単なるHTTP 200の成功と誤認しないよう，ホスト側のテストでは**本文全体とchunked終端を読み取ってから**成功と判定します．

```sh
# LLMを介さずに50回の温度取得を実行（標準ライブラリのみ）
python3 host/mcp_stress.py http://192.168.4.1:8080/mcp --count 50

# 隔離ネットワーク上で短い観測コマンドを繰り返す
python3 host/mcp_stress.py http://192.168.4.1:8080/mcp \
  --tool execute_command --command 'trace run hello' --count 50
```

障害が発生すると呼出し番号，所要時間，例外名を表示し，標準ではその場で停止します．
失敗したコマンドの再実行は行いません．`--continue-on-error`は**次の番号の要求を続行する**試験用オプションであり，書込みなど副作用のあるコマンドには使わないでください．

実機コンソールでは，試験前後に`cat /proc/net/memory`を比較してください．
`TCP_PCB_POOL`と`TCP_SEG_POOL`はlwIPのプール（現在／最大／確保失敗累積回数），
`TCP_ACCEPT_BUSY`は接続枠使用中の受理拒否，`TCP_WRITE_BACKPRESSURE`は送信キュー満杯，
`TCP_CLOSE_DEFERRED`と`TCP_CLOSE_RETRY`は遅延closeと再試行，`TCP_ASYNC_ERROR`はlwIPの非同期エラーの累積回数です．
`mcpd`コンソールの`HTTP send timeout (bytes left=...)`の行は送信待ち上限到達，
`HTTP send socket error (...)`はソケットAPIの失敗を表します．
`trace`の`write`（ソケット送信），`close`，`select`の戻り値と突き合わせると原因を絞り込めます．
カウンタは起動以来の累積値なので，必ず差分を取ってください．

小チャンクのまとめ書きと診断追加は，**実機での断続的な失敗が完全に解決したことを意味しません**．
USBコンソール等で収集した値とホスト側の失敗番号を併記して判断してください．


### `.key`との入力仕様の対応

`terminal_send`の`key`引数はカーネル内部で`key:`命令として処理されます．MCPクライアントのJSON形式は従来どおりです．`text`引数はJSON解析後のバイト列をPTY Masterへ直接送信します．`text`内のLFは保持し，`.key`の`echo`終端処理は適用しません．
