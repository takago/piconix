# PicoNIX エージェント操作ガイド（実験版）

Open WebUIなどのホスト側LLMエージェントから，MCP経由で**PicoNIX**を操作するためのガイドです．PicoNIXはRP2040上で動作する教育用UNIX風OSであり，**Linuxではありません**．`bash`，`sudo`，`apt`，`gcc`，`ssh`，完全なPOSIXシェルや標準Cライブラリが存在するとは仮定しないでください．

## コマンド実行ツールの有効化

信頼できる隔離された実験用ネットワークでのみ，次のように起動します．

```sh
mcpd --shell &
```

`execute_command`と`write_file`が`get_temperature`，`get_wifi_info`，`display_message`に追加されます．通常の`mcpd &`ではコマンド実行は公開されません．接続先は`http://<Pico-WのIP>:8080/mcp`です．現行実装はMCP `2026-07-28`の限定的なJSON応答方式で，SSEは実装していません．Open WebUIのバージョンによって直接接続できない場合があるため，既存のホストクライアントや互換ブリッジも検討してください．**認証，TLS，コマンド単位の権限管理はありません．** TCP 8080をインターネットや信頼できないネットワークに公開しないでください．

## `execute_command`の仕様

入力例：`{"command": "ps"}`．呼び出しごとに新しい非対話`sh -c`プロセスを起動し，パイプで標準出力と標準エラー出力を回収します．作業ディレクトリは毎回`/tmp`から始まり，`cd`の状態は保持されません．コマンド文字列は印字可能なASCIIで最大119バイト，出力は最大4,096バイト，タイムアウトは公称3秒です．出力の切り詰めは応答に明記します．HTTP/1.1のchunked転送を利用してJSONを逐次送信するため，`mcpd`内に4 KiBの出力バッファを確保しません．クライアントはchunked転送に対応する必要があります．接続が途中で切れた場合，JSON応答が不完全になる可能性があります．タイムアウト時には子孫プロセスが残る可能性があるため，必要なら`ps`で確認してください．**返り値は個々のコマンドの正確な終了コードを保証しません．** 端末入力を要求するコマンドは避けてください．

これは**サンドボックスではありません**．シェルはデバイス，RAMファイル，ネットワークにアクセスできます．ファイルの上書き，プロセスの終了，リセット，GPIOの変更などの前には利用者の許可を求めてください．

## `write_file`によるソースファイル作成

`mcpd --shell`では`write_file`も公開します．`/tmp`直下にASCIIテキストファイルを作成・上書きするMCPツールです．入力例：

```json
{"path":"/tmp/test.c","content":"int main(void) { return 0; }\n"}
```

`content`はJSON文字列で，`\n`，`\t`，引用符のエスケープを解釈します．**1ファイル2,048バイトまで**ですが，HTTPリクエスト本文の上限が**3,200バイト**なので，エスケープが多い場合は書き込める実データが少なくなります．追記や分割転送は行いません．対象パスは`/tmp/NAME`のみで，名前には英数字，`.`，`_`，`-`を使用できます．`/tmp`と`/etc`のRAMファイルは合計11個のスロットを共有します．既存ファイルは上書きされるので，利用者の許可を得てください．書き込み失敗時にはファイルが途中まで更新される場合があります．

エージェントは`write_file`でCソースを作成した後，`execute_command`で`cc /tmp/test.c`，`as /tmp/test.s`，`/tmp/a.out`の順に実行できます．ただし，コンパイル後のアセンブリも2 KiBに制限される場合があるため，小規模なプログラムから試してください．この一連の流れは実機未検証です．

## コマンド利用例

| 目的 | コマンド例 | 注意点 |
| --- | --- | --- |
| コマンド確認 | `help` | まず現行のコマンドを確認する |
| ファイル | `ls`，`pwd`，`cat /etc/rc` | RAMファイルが中心 |
| プロセス | `ps`，`lsof` | 同時実行数に制限 |
| ネットワーク | `ip`，`cat /proc/net/wifi` | Wi-FiはPico W向け |
| ネットワークメモリ | `cat /proc/net/memory` | lwIPの情報を表示 |
| デバイス | `cat /dev/adc/temp` | 温度センサを参照 |
| C言語 | `cc test.c`，`as test.s` | ISO C全体ではなくサブセット |
| パイプ | `ps | grep sh` | 最大3段，パイプとリダイレクトの併用不可 |
| トレース | `trace` | 詳細は`help`などを参照 |

既存のCソースをコンパイルする例です．

```text
execute_command({"command":"ls"})
execute_command({"command":"cc test.c"})
execute_command({"command":"as test.s"})
execute_command({"command":"/tmp/a.out"})
```

短い非対話コマンドを想定しています．`vi`，`edit`，対話モードの`python`，フォアグラウンドの`scroll`は適しません．Linuxと同じ引用符，グロブ，変数展開，ジョブ制御などは期待しないでください．

## Open WebUIへの登録手順

以下はOpen WebUIの管理画面を使う設定例です．画面項目名はバージョンによって多少変わる場合があります．まず**隔離した実験用ネットワーク**で行ってください．PicoNIXの`mcpd --shell`には認証・TLS・コマンド単位の権限制御がなく，ネットワークからファイル操作やデバイス制御を実行できます．Open WebUIの利用者・アクセス権も制限してください．

### 1．エージェントガイドをKnowledgeへ登録する

1. Open WebUIの **Workspace → Knowledge → Create** でKnowledgeを作成します（例：`PicoNIX Agent Guide`）．
2. この`docs/ja/guides/AGENT-GUIDE.md`をアップロードし，処理完了を確認します．英語のLLMを使う場合は英語版`docs/en/guides/AGENT-GUIDE.md`でも構いません．
3. **Workspace → Models** で操作用モデルを作成・編集し，Knowledgeに作成したコレクションを関連付けます．一時的な試験ならチャットで`#`からKnowledgeを参照する方法もあります．
4. 文書が短い場合は**Full Context**も選択肢です．ただし，Native function callingではモデルに関連付けたKnowledgeの全文が常に自動投入されるとは限らず，LLMがKnowledge検索ツールを呼ぶ必要がある場合があります．モデルがガイドを参照しない場合は，Knowledge検索ツールの有効化を確認し，システムプロンプトに「PicoNIX操作前にAgent Guideを参照する」と記述してください．

### 2．PicoNIXのMCPサーバを接続する

1. Pico WのIPアドレスと，`mcpd --shell &`が実験用ネットワークで起動していることを確認します．
2. Open WebUIの管理者として **Settings → Admin → Integrations → External Tool Servers → Add Connection** を開きます．
3. Typeに **MCP (Streamable HTTP)** を選択し，Server URLに`http://<Pico-WのIP>:8080/mcp`を指定して保存します．**OpenAPI**ではありません．DockerでOpen WebUIを動かしている場合は，コンテナからPico WのIPへ通信できる必要があります．
4. 接続先のツール一覧に`get_wifi_info`，`display_message`，`execute_command`，`write_file`などが現れることを確認します．利用するモデル・チャットでツール呼び出しを有効にし，必要なツールへのアクセス権を設定します．

> **互換性の注意：** PicoNIXの`mcpd`はMCPの限定的な実装です．使用中のOpen WebUIが直接接続できない場合があります．その場合は`host/mcp_llm_client.py`など既存のホスト側クライアントで先に通信を確認し，必要に応じて互換ブリッジを検討してください．接続に失敗しただけでPicoNIXの故障とは判断しないでください．

### 3．システムプロンプトを設定する

操作用モデルのSystem Promptへ，後述の「Open WebUIのシステムプロンプト用文面」を設定します．重要な制約（Linuxではない，破壊的操作には承認が必要，2 KiBのファイル上限）はKnowledgeの検索結果だけに依存せず，プロンプトにも記載してください．

### 4．読み取り専用コマンドで動作確認する

新しいチャットを開始し，「PicoNIXの稼働中プロセスを調べてください」と入力します．LLMが`execute_command`で`ps`を実行し，結果から回答できるか確認します．続いて`ip`や`help`を試します．失敗した場合はMCP接続・モデルのツール設定・Knowledgeの参照状況を個別に確認してください．`write_file`やデバイス変更は，この動作確認が済んでから，利用者の承認を得て試してください．

Open WebUI公式資料： [Knowledge](https://docs.openwebui.com/features/workspace/knowledge/) ／ [Models](https://docs.openwebui.com/features/workspace/models/) ／ [MCP接続](https://docs.openwebui.com/features/extensibility/mcp/)．

## Open WebUIのシステムプロンプト用文面

> あなたが操作するPicoNIXは，メモリ制限のある教育用UNIX風OSであり，Linuxではありません．MCPの`execute_command`では短く終了する観察用コマンドを優先してください．不明なときは`help`を確認し，存在しないオプションを推測しないでください．ファイル，プロセス，デバイス，ネットワーク設定の変更は利用者の明示的な承認を得てから実行してください．無限ループや対話型コマンドは実行せず，時間制限と出力制限を守ってください．結果を説明するときはコマンドの出力と推測を区別してください．PicoNIXが応答しなくなった場合は，破壊的操作を無条件に再試行しないでください．

## Linuxとの主な違い

- RP2040では6個の固定ユーザスロットを使用します．各スロットは24 KiBで，4 KiBのスタック予約を含みます．メモリ保護とUNIXユーザ権限はありません．
- シェルは小規模な独自実装です．3段までのパイプを扱えますが，Bash互換ではなく，パッケージ管理もありません．
- `/tmp`はRAMファイルです．`/proc`や`/dev`で状態の確認・物理デバイス操作ができます．
- `cc`はC言語のサブセットからThumb-1アセンブリを生成し，`as`が実行イメージに変換します．
- `mcpd`はユーザプロセスです．`sh`とその子プロセスを起動する空きスロットが必要です．
- `display_message`は`scroll -u 2001 &`へのUDP送信だけを行い，表示完了を保証しません．

## ログと検証

エージェントの判断理由，実行コマンド，日時，出力，エラーや切り詰めの状態は**ホスト側で記録**してください．PicoNIXの`trace`はシステムコールやスケジューリングの別の視点を提供します．ホストとPicoNIXの時計は同期しているとは限りません．

この機能は新しく追加した実験的な実装です．ARM向けの構文検査が成功しても，実機動作を保証しません．`execute_command({"command":"ps"})`から実機で確認し，まず読み取り専用の操作を試してください．
