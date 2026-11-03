# help コマンド

`help` は PicoNIX のコマンドを分類して一覧表示する独立した RAM アプリケーションです．

```text
piconix# help
piconix# help gpio
piconix# help as
piconix# help mb
piconix# help reset
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

## 主なコマンドとオプション

- `help pong`：`-2` はPongの2人対戦モードです．
- `help mb`：Moon Buggyには`-2`はありません．SPACE／`j`でジャンプし，`q`で終了します．
- `help fbctl`：`on`，`off`，`clear`，`status`，`test`．
- `help pwmctl`：`status`，`freq HZ`，`a DUTY`，`b DUTY`，`off`．
- `help stty`：`raw`／`cooked`，`icanon`，`echo`，`onlcr`と各`-`指定．
- `kitesys`：Kanazawa Institute of Tech. Embedded System Test Program．


### echo -e / -n

`echo -e` は `\n`，`\r`，`\t`，`\\`，`\033`（8進），`\x1b`（16進）を解釈します．`echo -n` は末尾の改行を省略します．シェルの引用符処理は限定的ですが，`echo -e '\033[?1049l'` の形式が使えます．
