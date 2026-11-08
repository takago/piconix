# ローカルLLMからPicoNIXのMCPツールを呼び出す

このガイドは現行の`userland/apps/mcpd.c`と`host/mcp_llm_client.py`に対応します．Linux PCのOpenAI互換LLMとPico WのMCPサーバを接続します．`get_wifi_info`と`display_message`は廃止済みで，現行の操作手順には使用しません．

[English](../../en/guides/MCP-LLM-CLIENT.md) ｜ [MCPの利用](MCP.md) ｜ [エージェント操作](AGENT-GUIDE.md) ｜ [サーバ開発](../development/apps/c/MCP-SERVER-DEVELOPMENT.md)

## 通信と公開ツール

```text
利用者 → ローカルLLM（OpenAI互換API）
               ↓ Function Calling
       host/mcp_llm_client.py
               ↓ MCP Streamable HTTP (TCP 8080)
          Pico Wのmcpd → /dev，/proc，/man，シェル
```

| 起動 | 公開するツール |
|---|---|
| `mcpd &` | `get_temperature`のみ |
| `mcpd --shell &` | `get_temperature`，`get_manual`，`execute_command`，`write_file` |

`get_manual()`は**`/man/README`の目次**を返し，`get_manual({"topic":"languages/c"})`は指定したページを返します．全文一括取得ではありません．`execute_command`は短い非対話コマンド用，`write_file`は`/tmp/NAME`にASCIIを上書きするツールです．ホストクライアントは`tools/list`から利用可能なツールを自動取得します．

## 接続手順

Pico Wを信頼できる隔離ネットワークに接続し，端末で`mcpd --shell &`を実行します．**このモードには認証・TLSがなく，シェル操作をネットワーク公開します．一般のLANやインターネットでは使用しないでください．** Linux側で以下を実行します．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3" openai
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_manual
```

`host/mcp_llm_client.py`の`LLM_URL`，`LLM_MODEL`，`LLM_API_KEY`，`MCP_URL`を実際のサーバに合わせて設定します．LLM側にはChat Completions APIとFunction Callingへの対応が必要です．接続先のIPはAPモードの例です．APIキーをGitに登録しないでください．

```sh
python host/mcp_llm_client.py
```

`exit`または`quit`で終了します．ホストのLLM受信には`stream=True`を使用します．モデルが思考テキストを公開する場合のみ`[THINK]`を表示できます．`LLM_REASONING_EFFORT`の対応値や実際の出力はサーバ／モデルに依存します．MCPのHTTP応答のチャンク転送とLLM応答のストリーミングは別です．

## 操作例（想定されるツール呼び出し）

```text
質問: PicoNIXのIPアドレスを調べて
→ get_manual() → execute_command({"command":"ip"})

質問: LEDにハートを表示して
→ get_manual({"topic":"led-patterns"})
→ execute_command({"command":"echo heart > /dev/fbchar"})

質問: 実行中のプロセスを調べて
→ execute_command({"command":"ps"})
```

これは実測ログではなく例です．スクロール表示は`echo HELLO | scroll`でも試せますが，パイプはタスクスロットを消費します．対話型・全画面の`vi`，`edit`，`mem`，`watch`，`fd`などをMCPから起動しないでください．

`host/mcp_client.py --tool all`は，読み取り専用の`get_temperature`と`get_manual`だけを対象にします．書込み・コマンド実行を自動実行しません．ファイル・GPIO・ネットワークの変更前には利用者の承認を得てください．MCPは試作版であり，モデルのツール選択や応答の正確性を保証しません．
