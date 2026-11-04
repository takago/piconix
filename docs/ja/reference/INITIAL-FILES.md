# 初期RAMファイルと /etc/rc

*技術リファレンス*

PicoNIXの起動時RAMファイルは，ホスト側の`userland/fs/`以下に置いた通常のファイルから生成します．ビルド時にFlashへ埋め込み，起動時にRAMファイルへ展開します．カーネルのCソースに初期HTMLやサンプルコードを書き込む必要はありません．

## ホスト側のディレクトリ

```text
userland/fs/
├── etc_pico/
│   └── rc
├── etc_picow/
│   ├── fbcheck.html
│   ├── index.html
│   ├── pwm.html
│   └── rc
└── tmp_common/
    ├── test.s
    ├── test.py
    └── wave.py
```

ディレクトリ名は「配置先（`etc`または`tmp`）＋対象ボード（`common`，`pico`，`picow`）」を表します．必要に応じて`tmp_pico/`と`tmp_picow/`も使用できます．`common`は両ボードで使い，ボード別ファイルに同じ配置先と名前があれば，ボード別の内容を優先します．例えば`etc_picow/rc`はPico W上の`/etc/rc`になります．

**これらはホスト側の分類フォルダであり，PicoNIX上に新しいディレクトリを作る機能ではありません．** 認識するフォルダは上記の6種類だけで，その中にサブフォルダを設けると生成エラーになります．未知のフォルダもエラーになります．

## 容量・ビルド

1回のビルドで選択される初期ファイルは最大11個，各ファイル2048バイト以下です．内部VFSの名前（`etc/rc`など）は31バイト以内で，配置先は`/etc`または`/tmp`に限ります．`tools/generate_initial_fs.py`が生成時に制限を検査します．

ファイルを追加・変更したら，対象ボード向けのファームウェアを再ビルドして書き込みます．初期RAMファイルは起動時にFlashの内容から復元されるため，実機上で編集した内容は電源再投入後には残りません．

`/tmp/wave.py`はBH1750照度センサー（I²Cアドレス`0x23`）と8×8 LEDマトリクスを使用するサンプルです．実行にはセンサーの接続が必要です．

## /etc/rc と起動処理

独立した`init`プロセスは設けず，**PID 1の最初の`sh`が`/etc/rc`を一度だけ実行**します．Telnetで起動する別のシェルや入れ子のシェルでは再実行しません．既存シェルのコマンド処理を再利用し，空行と`#`で始まる行は読み飛ばします．起動スクリプトでは単純なコマンドと末尾の`&`を使用してください．この起動時処理はパイプやリダイレクトを解釈しません．

現在の最小構成では，Pico用の`etc_pico/rc`には起動メッセージ，Pico W用の`etc_picow/rc`にはHTTPサーバの起動と起動メッセージが記述されています．Pico版は4ファイル（`/etc/rc`と3つの`/tmp`サンプル），Pico W版は7ファイル（`/etc/rc`，3つのHTML，3つの`/tmp`サンプル）が初期配置されます．

Pico用：

```sh
echo "Welcome to PicoNIX!"
```

Pico W用：

```sh
httpd &
echo "Welcome to PicoNIX!"
```

関連：[ファイルシステム](FILESYSTEM.md)・[HTTPサーバ](network/HTTPD.md)・[ネットワーク](network/NETWORK.md)．

---

**関連リンク：** [リファレンス目次](README.md) ｜ [教科書総目次](../learn/README.md) ｜ [English](../../en/reference/INITIAL-FILES.md)
