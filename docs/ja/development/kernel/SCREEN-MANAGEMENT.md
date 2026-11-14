# スクリーン管理モジュール

*カーネル開発・読解ガイド — `kernel/screen.c`，`kernel/screen.h`*

PicoNIXのスクリーン管理は**カーネル内の独立したモジュール**です．別のユーザプロセスではないため，ユーザタスクスロットを消費しません．TTY・PTYは端末のバイト入出力を担い，`screen.c`は80×24文字の画面状態，ANSI制御の一部，CDC仮想コンソールの描画状態，画面操作に関するキー入力の解釈を担います．

## 1．担当範囲と境界

| `screen.c`／`screen.h`の担当 | `kernel.c`などに残る担当 |
|---|---|
| `vt_screen_t`・`pty_screen[]`・`cdc_screen[]`の保持 | PTY Master/SlaveのFD・参照数・入出力リング |
| `screen_byte()`によるANSI画面解釈 | TTY入力キュー・行編集・起床処理 |
| `screen_read()`による画面ストリーム生成 | `/dev/pts/N.screen`のVFS判定・FD位置の保持 |
| `screen_key_decode()`による`.key`命令の変換 | `.key`のPTY入力への配送・エラー応答 |
| `screen_cdc_poll()`による段階的な再描画 | USB CDC送信リングと`console_queue()` |
| `screen_cdc_key_feed()`によるCDC操作キー判定 | TTYへの文字配送・コンソール切替時のシェル生成 |

**重要：** 画面の意味を解釈する仕事と，バイトを届ける仕事を分離しています．`screen.c`は`spawn()`やVFSのパス解決を実行しません．

```text
  sh / vi / ゲーム
          │ write/read
          ▼
  TTY・PTY・VFS（kernel.c） ──── USB CDC送信リング
          │        ▲                 ▲
          ▼        │                 │
       screen.c ───┴── 画面再描画データ
   ANSI解析・画面状態・キー解釈
          ▲
 /dev/pts/N.screen・/dev/pts/N.key （VFS経由）
```

## 2．画面状態とANSI解析

- `vt_screen_t`は文字セル`cells[24][80]`，反転属性ビット列`inverse`，カーソル位置，CSI解析状態，更新世代`revision`を保持します．
- `pty_screen[SCREEN_PTY_COUNT]`と`cdc_screen[SCREEN_CDC_COUNT]`は各2画面の状態です．**PTYとCDCで合計4画面**を保持します（同時に表示する数とは異なります）．
- `screen_reset()`・`screen_clear()`は画面状態を初期化します．`screen_byte()`は端末出力を1バイトずつ解釈します．
- `screen_byte()`はカーソル位置照会`ESC[6n`を検出すると通知値を返し，実際の応答をTTY入力へ配送するのは`kernel.c`の`screen_cursor_report()`です．
- 対応するANSI制御は実装された範囲に限られ，汎用ターミナルエミュレータとの完全互換を意味しません．

**読み進める順序：** `kernel/screen.h`の`vt_screen_t` → `screen_reset()` → `screen_byte()` → `kernel/kernel.c`の`pty_screen_byte()`・CDC出力処理．

## 3．PTYの`.screen`と`.key`

`/dev/pts/N.screen`と`/dev/pts/N.key`はVFSから認識されるデバイスノードです．`screen.c`自身がファイル記述子を割り当てるわけではありません．

### `.screen`：画面の読取り

`pty_screen_read()`（`kernel.c`）が対象PTY画面とFD位置を渡し，`screen_read()`が80×24画面を文字と必要なANSI属性に変換して返します．画面取得には反転表示用のSGR 7/0と，カーソル位置の下線用SGR 4/24が含まれます．FD位置は呼出側で保存するため，途中から続けて読めます．更新中の画面を読む場合，原子的なスナップショットは保証されません．

### `.key`：入力命令の変換

`pty_key_write()`（`kernel.c`）が`screen_key_decode()`を呼び，検証・変換済みのバイト列を`pty_master_rw()`へ渡します．**1回の`write()`が1命令**であり，複数の`write()`を結合しません．

```sh
echo 'text:ls\n' > /dev/pts/0.key
echo 'key:ENTER' > /dev/pts/0.key
echo 'key:CTRL_C' > /dev/pts/0.key
cat /dev/pts/0.screen
```

- `key:`は印字可能な1文字または定義済みのキー名（`ENTER`，`ESC`，`UP`，`CTRL_A`〜`CTRL_Z`など）を指定します．
- `text:`では`\n`をLF，`\\`をバックスラッシュとして解釈します．それ以外のバックスラッシュ指定はエラーです．
- `echo`由来の末尾LF，およびその直前のCRは命令終端として除去します．文字列末尾の改行を送りたい場合は`text:ls\n`のように明示します．
- 出力は最大127バイトです．入力形式が不正ならPTYへ配送する前に`OS_EINVAL`となります．ただし，PTYの入力キュー混雑による部分受付は別問題です．
- PTYのMasterとSlaveが利用可能である必要があります．番号0は必ずしも使用中とは限りません．

詳しいコマンドと対応キーは[PTYデバイス仕様](../../reference/devices/PTY.md)を参照してください．

## 4．CDC仮想コンソールの再描画

`cdc_screen[0]`・`cdc_screen[1]`は独立した画面を保持します．単画面表示または縦2画面の表示モードと選択中画面は`cdc_active`・`cdc_dual_mode`で管理します．

1. `screen_cdc_begin_repaint()`で再描画の状態を初期化します．
2. `screen_cdc_poll()`はアイドル側から呼ばれ，画面状態に応じて少しずつ描画データを生成します．
3. 出力データはコールバック`screen_cdc_emit_t`を通じて，`kernel.c`の`console_queue()`からUSB送信キューに格納します．
4. キューに空きがない場合は送信を無理に進めず，次のポーリングで再開します．

現行実装では，**1回の処理で生成する出力は最大96バイト**となるように制限されています．再描画処理を修正する際は，送信キューの満杯処理や背景画面の更新検出を壊さないでください．

## 5．CDCのキー入力とシェル起動

`screen_cdc_key_feed()`と`screen_cdc_key_timeout()`は，受信バイトを通常TTY入力と画面操作に分類します．`Ctrl+T`，`Ctrl+G`，`Ctrl+矢印`の判定，未完成ESC列のタイムアウト，入力開始時のTTY番号の保持を扱います．

解析結果の`screen_cdc_key_result_t`には，元のTTYへ配送する文字列，単独の入力文字，画面操作を表す`action`が含まれます．`kernel.c`の`cdc_key_dispatch()`が結果に応じて入力をTTYへ渡すか，画面切替を指示します．**2番目のシェルが必要かの判断と`spawn_in_slot()`による起動は`kernel.c`の`cdc_vconsole_switch()`に残します．**

## 6．MCPとの関係

`mcpd`は独立したユーザプログラムです．MCPの`terminal_send`では，通常の`text`は既存のPTY Master経路で送信し，特殊な`key`指定は`.key`経路を利用します．`screen.c`にMCPやHTTPの解析処理を持ち込む必要はありません．

## 7．変更時の確認

- CMakeの`kernel/screen.c`組込み設定と`screen.h`の型・宣言を確認する．
- 画面制御：スクロール，カーソル移動，反転表示，`/dev/pts/N.screen`の読取りを検査する．
- キー入力：`key:`・`text:`の正常系，不正エスケープ，長さ超過を検査する．
- CDC：`Ctrl+T`，`Ctrl+G`，`Ctrl+矢印`，ESC単独，縦2画面，再描画を確認する．
- PTY・MCP：`terminal_send`の`key`と`text`，`terminal_read`を確認する．
- Pico SDK 2.3.1でビルドし，カーネルの56 KiB領域，ユーザアプリ24 KiB領域，ROMサイズを確認する．

**ソース上の入口：** [`KERNEL-READING-GUIDE.md`](KERNEL-READING-GUIDE.md) ／ [カーネル拡張ガイド](KERNEL-DEVELOPMENT.md) ／ [PTY仕様](../../reference/devices/PTY.md) ／ [MCP仕様](../../reference/commands/MCP.md)．

[開発ガイド一覧](../README.md) ｜ [English](../../../en/development/kernel/SCREEN-MANAGEMENT.md)
