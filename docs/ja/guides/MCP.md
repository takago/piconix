# MCP Streamable HTTP実験サーバ（`mcpd`）

`mcpd` は，Pico W上のPicoNIXをMCPサーバとして利用する実験用ユーザプログラムです．カーネルには変更を加えず，TCP 8080番で `POST /mcp` を受け取ります．既存のTCP 80番 `httpd` とは独立しています．

**実験段階です．認証とTLSはありません．外部ネットワークに公開しないでください．** `Origin` ヘッダがあるリクエストはすべて拒否します．信頼できる隔離されたLANで，PC上のコマンドラインクライアントからのみ試してください．

## 対応範囲

- MCP仕様 `2026-07-28` のStreamable HTTPの一部に対応．HTTP POSTとJSON-RPC応答を使用します．
- `server/discover`，`tools/list`，`tools/call` の3メソッドを実装しています．
- `get_temperature` は `/dev/adc/temp` から温度を読み取ります．
- `get_wifi_info` は `/proc/net/wifi` を読み取ります（長い出力は先頭255バイトまで）．
- SSE，古いMCPの `initialize`，認証・TLS，通知，複数クライアント同時処理，完全なJSON/HTTP検証には対応していません．公式MCP Python SDK v2との基本的なツール列挙・実行は実機で確認済みです．
- 固定長のHTTP要求バッファは3072バイト，JSON本文の上限は1536バイトです．1接続ずつ処理します．

## 起動

Pico Wでビルドし，ネットワークを接続した後で起動します．

```sh
mcpd &
```

`httpd` を置き換える必要はありません．`mcpd` は標準の起動スクリプトには追加していません．

## PCからの動作確認

以下の `PICO_IP` はPico WのIPアドレスに置き換えます．

```sh
PICO_IP=192.168.4.1
curl -i -X POST "http://$PICO_IP:8080/mcp" \
  -H 'Content-Type: application/json' \
  -H 'Accept: application/json, text/event-stream' \
  -H 'MCP-Protocol-Version: 2026-07-28' \
  -H 'Mcp-Method: server/discover' \
  --data '{"jsonrpc":"2.0","id":1,"method":"server/discover","params":{"_meta":{"io.modelcontextprotocol/protocolVersion":"2026-07-28","io.modelcontextprotocol/clientInfo":{"name":"curl-test","version":"1.0"},"io.modelcontextprotocol/clientCapabilities":{}}}}'
```

`Mcp-Method` とJSON本文内の `method` は一致させます．`tools/call` では，追加で `Mcp-Name` ヘッダと `params.name` が必要です．例えば温度測定では `Mcp-Method: tools/call`，`Mcp-Name: get_temperature`，および `"name":"get_temperature"` を指定します．

MCPの仕様に従ったすべてのクライアントで動くことを保証するものではありません．まずこのHTTPリクエストに200が返ることを確認してください．

## Linux上のPythonクライアント

`host/mcp_client.py` は公式MCP Python SDK v2を使用する検証用クライアントです．LLMは使わず，ツール一覧の取得とツール呼び出しを行います．

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3"
python host/mcp_client.py http://192.168.4.1:8080/mcp
```

IPアドレスはPico Wに割り当てられたものに置き換えます．引数を省略した場合の接続先は `http://192.168.4.1:8080/mcp` です．

```sh
# ツール一覧だけ確認する
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool none

# 温度だけ取得する
python host/mcp_client.py http://192.168.4.1:8080/mcp --tool get_temperature

# 初期探索を省略して2026-07-28版を指定する
python host/mcp_client.py http://192.168.4.1:8080/mcp --mode 2026-07-28
```

SDKの `mode=auto` は `server/discover` を試し，対応しなければ旧形式へ切り替えるため，PicoNIX試作版で問題が起きた場合は `--mode 2026-07-28` も試してください．これは公式SDKと試作版サーバの相互運用性を確認するためのコードであり，2026-11-06にPico W実機との接続・ツール呼び出しを確認しました．未認証HTTPのため隔離されたLANでのみ利用してください．

## 実装と検証

- アプリの登録は `CMakeLists.txt`，`kernel/main.c`，`userland/apps/help.c` に追加しています．プログラムIDは49です．
- `-Oz`，24 KiBスロット，4 KiBスタックの既存方針を維持します．
- ARM Cortex-M0+向けの構文検査，ホスト上でのJSON-RPC応答テストは通過しました．
- テスト用の代替EABI関数を使った暫定リンクでは，PICイメージ7704バイト，BSS4776バイト，4 KiB予約スタックで24 KiBスロット内に収まりました．**このサイズは試作時の暫定測定値です．その後，Pico WでのMCP通信は実機確認済みですが，正式なイメージサイズの再計測や長期安定性試験はここでは確認していません．**

## ローカルLLMとの連携

[LLMからMCPツールを呼び出す実習](MCP-LLM-CLIENT.md)では，`host/mcp_llm_client.py`を使用します．2026-11-06にLinux上のローカルLLMから温度・Wi-Fi情報を取得する実機実験に成功しました．LLMサーバのIP・モデル名・APIキーはプレースホルダです．
