# Python風インタプリタ

[English](PYTHON-EN.md) | [概要へ戻る](README-JP.md)

`ram_apps/python.c`はPicoNIX独自の**小型Python風言語**です．CPythonやMicroPythonではありません．OmikujiとRAMスロット3を共有し，両者は同時実行できません．

## 1．対話モード

```text
piconix# python
>>> x = 10
>>> print(x + 5)
15
>>> exit()
piconix#
```

引数なしの`python`で対話モードに入り，`exit()`でShellへ戻ります．入力は行単位です．`if`や`while`のブロックはインデントして入力し，空行で確定します．

## 2．ファイルを実行する

```text
piconix# cat > /tmp/test.py
print(1 + 2)
```

最後に空行で`Ctrl+D`を入力し，ファイル保存を終えます．続いて実行します．

```text
piconix# python /tmp/test.py
3
```

`cd /tmp`したShellからは`python test.py`でも実行できます．スクリプトは`/tmp`のRAMファイルに限られ，電源を切ると消えます．ファイル実行時には対話バナーを表示しません．

## 3．対応する機能

| 分類 | 内容 |
|---|---|
| 値と計算 | 符号付き32ビット整数，`+ - * / %`，比較，括弧 |
| 制御 | 代入，`if`／`else`，`while`，スペースによるインデント，`#`コメント |
| 入出力 | `print(...)`，整数入力の`input(...)`，`int(input(...))` |
| 待機 | `sleep(ミリ秒)`（0～60000） |
| GPIO | `gpio`，`gpio_read`，`gpio_mode`，`gpio_pull` |

変数は最大16個，ソースは512バイト／非空行64行，ブロックは8段までです．`while`は1文につき最大10000回の反復に制限されています．浮動小数点，リスト，ユーザ定義関数，`for`，`import`，文字列変数はありません．

## 4．GPIOの例

`/tmp/led.py`に次の内容を保存し，`python /tmp/led.py`で実行します．

```python
i = 0
while i < 5:
    gpio(25, 1)
    sleep(250)
    gpio(25, 0)
    sleep(250)
    i = i + 1
```

`gpio_mode(pin, 0)`が入力，`gpio_mode(pin, 1)`が出力です．`gpio_pull(pin, 0/1/2)`はそれぞれ無効／プルアップ／プルダウンです．UART用のGPIO 0，1，4，5は操作できません．ボードや接続回路の電気的制約にも注意してください．

## 5．バックグラウンド実行

```text
piconix# python /tmp/led.py &
piconix# python /tmp/led.py > /tmp/output.txt &
```

`python &`は使えません．対話モードがShellとコンソール入力を取り合うためです．背景スクリプトからのコンソール入力にも制約があります．
