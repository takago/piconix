# help コマンド

`help` は PicoNIX のコマンドを分類して一覧表示する独立した RAM アプリケーションです．

```text
piconix# help
piconix# help gpio
piconix# help as
piconix# help mb
piconix# help reset
piconix# help cat
```

`help` は登録済みのRAMコマンドとシェル組込みコマンド（`cd`，`getty`，`exit`，`logout`）を分類して表示します．`grep` も一覧と `help grep` の個別説明に含まれます．コマンドの追加・削除に伴い一覧は更新が必要です．詳細なデバイス操作や制限事項は個別のドキュメントを参照してください．

`help` はシェルの組込み処理ではありません．シェルから通常の外部コマンドと同じ仕組みで起動されます．

## ピン配置とLED表示を調べる

```text
piconix# help pins
piconix# pins
piconix# help fbed
piconix# fbed
```

`pins`は標準の物理ピン配置を表示します．現在のピン機能の定義は`kernel/pins.h`にあり，実際のGPIO利用状態は`gpio`でも確認できます．`fbed`は8×8のフレームバッファを対話的に編集するコマンドです．カーソルキーで位置を選び，SPACEで点灯状態を切り替え，`c`で消去し，`q`で終了します．詳しい配線と点灯の説明は[デバイスガイド](../reference/devices/DEVICES.md)を参照してください．

`python3`はPicoNIX内のコマンドではありません．Pythonスクリプトは`python /tmp/test.py`のように起動します．ホストPCで実行する`python3 tools/...`とは区別してください．

## ファイルの表示と行番号

`cat FILE`はファイルをそのまま表示し，`cat -n FILE`は空行を含むすべての行に1から始まる番号を付けます．ファイル名を省略すると標準入力から読み込むため，`echo hello | cat -n`も使えます．書式は`help cat`で確認できます．実習例は[標準入出力](IO-IPC.md)を参照してください．

`ls -a`は`.`と`..`も表示し，`ls -l`は種別，ID，サイズを表示します．`edit`では`/tmp`または`/etc`直下のRAMファイルを指定します．`dd`の`if=`と`of=`を省略すると標準入出力を使い，`bs`は1〜256バイトです．`grep`は正規表現ではなく文字列を検索し，128バイトを超える行は検索対象から除外します．

## 主なコマンドとオプション

- `help pong`：`-2` はPongの2人対戦モードです．
- `help mb`：Moon Buggyには`-2`はありません．SPACE／`j`でジャンプし，`q`で終了します．
- `help fbctl`：`on`，`off`，`clear`，`status`，`test`．
- `help pwmctl`：`status`，`freq HZ`，`a DUTY`，`b DUTY`，`off`．
- `help stty`：`raw`／`cooked`，`icanon`，`echo`，`onlcr`と各`-`指定．
- `kitesys`：Kanazawa Institute of Tech. Embedded System Test Program．


### echo -e / -n

`echo -e` は `\n`，`\r`，`\t`，`\\`，`\033`（8進），`\x1b`（16進）を解釈します．`echo -n` は末尾の改行を省略します．シェルの引用符処理は限定的ですが，`echo -e '\033[?1049l'` の形式が使えます．

## Pico Wのネットワークコマンド

`ping [-c COUNT] IPv4`は既定で4回のEcho Requestを送ります．`192.168.4.2`のような数値のIPv4アドレスを指定してください．`tcpdump [-x] [-c COUNT]`はIPv4のTCP/UDP/ICMPを観察します．`-x`を指定すると字下げした2行目にデータ長と先頭最大16バイト（HEX・ASCII）を表示します．どちらもPico W専用です．詳しくは[ネットワーク実習](NETWORKING.md)を参照してください．

`ip addr`でPico WのループバックとWi-Fiインターフェースを確認できます．`edit`は未保存の変更がある場合，Ctrl+Qを2回押すと破棄して終了します．保存するときはCtrl+Sです．Ctrl+Cでも保存せず終了できます．

ファイルを複製するときは`cp SRC DST`を使用します．例：`cp /etc/index.html /tmp/index.html`．`/tmp`と`/etc`のRAMファイル以外は対象外です．
