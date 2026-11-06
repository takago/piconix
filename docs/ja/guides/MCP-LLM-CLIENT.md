# ローカルLLMからPicoNIXのMCPツールを呼び出す

このガイドでは，Linux PC上のOpenAI互換LLMと，Pico W上のPicoNIX `mcpd`を接続します．`host/mcp_llm_client.py`は**非ストリーミング**の小さなPythonサンプルです．LLMは通常の会話にも応答し，必要な場合だけMCPツールを選択して実行します．

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

LLMはMCPへ直接アクセスせず，Pythonプログラムがツール一覧を取得し，LLMのFunction CallingをMCPの`tools/call`へ橋渡しします．LLMの`stream=True`とMCPの*Streamable HTTP*は別の機能です．このプログラムではLLMの出力を一括で受け取ります．

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
