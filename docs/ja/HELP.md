# help コマンド

`help` は PicoNIX のコマンドを分類して一覧表示する独立した RAM アプリケーションです．

```text
piconix# help
piconix# help gpio
piconix# help as
piconix# help mb
piconix# help reset
```

`help` は全登録済み RAM コマンド（29個）とシェル組込みコマンド（`cd`，`getty`，`exit`，`logout`）を含みます．詳細なデバイス操作や制限事項は個別のドキュメントを参照してください．

`help` はシェルの組込み処理ではありません．シェルから通常の外部コマンドと同じ仕組みで起動されます．

## 補足（コマンド実装と照合）

- `help pong`：`-2` はPongの2人対戦モードです．
- `help mb`：Moon Buggyには`-2`はありません．SPACE／`j`でジャンプし，`q`で終了します．
- `help fbctl`：`on`，`off`，`clear`，`status`，`test`．
- `help pwmctl`：`status`，`freq HZ`，`a DUTY`，`b DUTY`，`off`．
- `help stty`：`raw`／`cooked`，`icanon`，`echo`，`onlcr`と各`-`指定．
- `kitesys`：Kanazawa Institute of Tech. Embedded System Test Program．
