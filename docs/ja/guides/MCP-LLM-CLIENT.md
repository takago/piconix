> **2026-11-08 更新：** `host/mcp_llm_client.py`はLLM応答を常に`stream=True`で受信し，通常回答を逐次表示します．OllamaのOpenAI互換APIには既定で`reasoning_effort="low"`を指定します．モデル／サーバが`reasoning_content`（または`reasoning`）を送る場合，内容を灰色の`[THINK]`として逐次表示します．ツール呼び出しは分割された差分を組み立てて実行します．Thinkingの内容がAPIから提供されない場合，`[THINK]`は表示されません．この機能はホスト側のみの変更です．

**設定**：`LLM_REASONING_EFFORT = "low"`．LLMサーバの機能に応じて`none`，`medium`，`high`などへ変更できます．実際の思考量はモデルとOllamaのバージョンに依存します．`[THINK]`はモデルが公開したテキストであり，内部の判断過程すべてを示す保証はありません．ストリーミングはLLMからの応答に適用され，MCPのStreamable HTTPとは別の機能です．

# ローカルLLMからPicoNIXのMCPツールを呼び出す

> **2026-11-08の仕様変更：** `get_wifi_info`と`display_message`は現行`mcpd`から削除されました．以下の旧ツールによる実例は開発当時の記録です．現行の`mcpd --shell`では引数なしの`get_manual()`で全文を取得し，`execute_command`から`ip`や`echo HELLO | scroll`を実行してください．詳細は[エージェントガイド](AGENT-GUIDE.md)．

## 現行のMCPエージェント操作（2026-11-08）

`mcpd --shell &`で起動した場合，MCPツールは`get_temperature`，`execute_command`，`write_file`，`get_manual`です．`get_wifi_info`と`display_message`は廃止されました．通常の`mcpd &`ではシェル操作ツールを公開しません．

`get_manual`は**引数なし**で全文を返します．`host/mcp_llm_client.py`は起動時に`tools/list`でツールを自動取得し，ツールの種類を固定していません．新しいシステムプロンプトでは，PicoNIXを操作する前に`get_manual()`を取得し，必要なら`execute_command({"command":"help trace"})`などで詳細を確認するよう指示します．LLMが必ずその順序を守る保証はありません．

```sh
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_manual
python host/mcp_llm_client.py
```

直接MCP接続を確認する`mcp_client.py`も，`get_manual`を空引数`{}`で呼び出します．`--tool all`は利用可能な読み取り専用ツール`get_temperature`と`get_manual`を呼び，`execute_command`と`write_file`は自動実行しません．

エージェントへの操作例：

```text
PicoNIXのマニュアルを読んで，実行中のプロセスを調べてください．
PicoNIXのネットワーク状態をipで調べてください．
```

LED表示は専用MCPツールや`nc`ではなく，基本的には`execute_command`経由で`echo HELLO | scroll`を使います．ただし，パイプは追加のプロセススロットを消費します．`mem`，`vi`，`watch`などの対話型・全画面型コマンドはMCPから起動しないでください．`mcpd --shell`は認証なしのコマンド実行を公開するため，隔離ネットワークでのみ使用してください．

以下の旧ツールの実行例は開発履歴として残しています．現行の操作手順ではありません．



このガイドでは，Linux PC上のOpenAI互換LLMと，Pico W上のPicoNIX `mcpd`を接続します．`host/mcp_llm_client.py`は**常時ストリーミング**の小さなPythonサンプルです．LLMは通常の会話にも応答し，必要な場合だけMCPツールを選択して実行します．

**実機で確認済み（2026-11-06）：** PicoNIXのAPモード（`192.168.4.1`）で，公式MCP Python SDK v2を使ったツール列挙と実行，さらにローカルLLMによるツール選択と回答生成に成功しました．これは基本機能の確認であり，MCP全仕様への準拠や長期安定性の検証ではありません．

[English](../../en/guides/MCP-LLM-CLIENT.md) ｜ [MCPサーバの利用](MCP.md) ｜ [MCPサーバ開発実習](../development/apps/c/MCP-SERVER-DEVELOPMENT.md)

## 1．通信の流れ

```text
Linux PC                   Pico W / PicoNIX
ユーザ                       mcpd (TCP 8080)
  ↓ 質問                       ↑ ↓
ローカルLLM                 /dev/adc/temp
  ↓ ツール呼び出し候補         /proc/net/wifi
host/mcp_llm_client.py ── MCP Streamable HTTP ──> mcpd
  ↑ ツール実行結果  <───────────────────────────┘
ローカルLLM → 自然言語による回答
```

LLMはMCPへ直接アクセスせず，Pythonプログラムがツール一覧を取得し，LLMのFunction CallingをMCPの`tools/call`へ橋渡しします．LLMの`stream=True`とMCPの*Streamable HTTP*は別の機能です．このプログラムではLLMの出力を逐次表示します．

## 2．準備

Pico Wで`mcpd &`を実行し，Linux PCから接続できる状態にします．MCP単体の動作は，LLMを組み合わせる前に確認してください．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3" openai
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none
```

`host/mcp_llm_client.py`の先頭にある以下の設定を，利用するLLMサーバに合わせて変更します．`######`は未設定を表すプレースホルダです．

```python
LLM_URL = "http://######:11434/v1"
LLM_MODEL = "######"
LLM_API_KEY = "######"
LLM_REASONING_EFFORT = "low"

MCP_URL = "http://192.168.4.1:8080/mcp"
```

LLMサーバはOpenAI互換のChat Completions APIとFunction Callingに対応している必要があります．`11434`は例示のポートです．`MCP_URL`はPicoNIXのAPモードで実際に確認された接続先なので，初期値を維持しています．ネットワーク構成が異なる場合だけ変更します．実際のAPIキーをGitにコミットしないでください．

## 3．実行

```sh
python host/mcp_llm_client.py
```

実機で確認した対話例（測定値と接続状態は実行時に変わります）です．

```text
User > こんにちは
LLM > こんにちは！何かお手伝いできることはありますか？

User > PicoNIXの状態を教えて
Tool: get_wifi_info
Result: MODE: AP
...
IP: 192.168.4.1
...
Tool: get_temperature
Result: 31.8

LLM > PicoNIXの現在の状態は以下の通りです．
... 温度31.8℃，APモード，IPアドレス192.168.4.1 ...
```

`exit`または`quit`で終了します．通常の質問ではMCPを呼ばず，PicoNIXの温度やWi-Fi情報が必要ならLLMが`get_temperature`や`get_wifi_info`を選びます．LLMによる判断のため，呼び出し順序や回答文は毎回同じとは限りません．

## 4．ソースの読み方

1. `Client(MCP_URL, mode="auto")`でMCP接続を確立する．
2. `mcp.list_tools()`の名前・説明・`input_schema`を，OpenAI互換の`tools`へ変換する．
3. `llm.chat.completions.create()`で通常の会話またはツール呼び出し要求を受け取る．
4. `message.tool_calls`があれば，JSON引数を解析して`mcp.call_tool()`を実行する．
5. ツール結果を`role="tool"`として会話履歴へ追加し，LLMにもう一度回答させる．
6. ツール呼び出しがなければ，最終回答を表示して次の質問を待つ．

MCP SDK v2のPython属性は`input_schema`，`is_error`です．`inputSchema`，`isError`ではありません．ツール結果は`result.content`のテキスト要素から取り出します．

## 5．制約と安全上の注意

- MCPサーバは認証・TLSを持たない試作版です．信頼できる隔離ネットワーク内でのみ使用してください．
- 現在の2ツールは読み取り専用です．GPIO制御などの書き込みツールを加える際は，権限・対象・入力値・操作確認の制約が必要です．
- サンプルにはツール呼び出しの回数上限がありません．モデルが同じツールを繰り返す場合は終了しないことがあるため，実運用では制限を設けてください．
- モデルが測定値に根拠のない評価を添える可能性があります．実際の測定結果とLLMの解釈は区別してください．
- LLMが利用するOpenAI互換APIやモデルにより，Function Callingが動作しない場合があります．

## 6．関連する演習

[演習資料](../exercises/README.md#mcpとllmの役割を比較する)に，MCPとLLMの役割を比較する課題を掲載しています．


## LED表示と複数ツールの連携

`scroll -u 2001`をPicoNIX上で起動してから，次を試します．`scroll`はUDP 2001番の通常のASCII文字列をリングバッファに格納します．ACKは返しません．

```sh
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool display_message --text "HELLO PICONIX"
python host/mcp_llm_client.py
```

LLM版では次のように質問します．

```text
User > PicoNIXのIPアドレスを調べて，LEDマトリクスにスクロール表示して
Tool: get_wifi_info
Result: ... IP: 192.168.4.1 ...
Tool: display_message
Result: ...
```

これは想定される呼び出し順序を示す例であり，実測ログではありません．IPアドレスは環境により異なります．LLMは`get_wifi_info`の結果を読んでから`display_message`に`text`を指定します．Python側にIPアドレス表示専用の処理は追加していません．

`display_message`の成功はUDP送信処理の成功のみを表し，`scroll`への配送やLED表示は保証されません．`scroll`が起動していなくてもUDP送信が成功する場合があります．UDPは認証されないため，信頼できるネットワークで使用してください．

`host/mcp_client.py`の`--tool all`はLED表示も実行します．表示を変更したくない場合は`--tool none`または読み取り専用ツールを指定してください．
