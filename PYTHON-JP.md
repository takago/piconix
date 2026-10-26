# Tiny Python風インタプリタ（スロット3）

[English](PYTHON-EN.md)

PicoNIXには，`ram_apps/python.c`に実装された小型のPython風インタプリタがあります．`omikuji`と同じRAMスロット3（`0x20030000`）を使用するため，両者を同時には実行できません．

引数なしの`python`は対話モード（REPL）を起動します．終了するには`exit()`を入力します．

```text
piconix# python
PicoNIX Tiny Python (Python subset)
Type exit() to quit.
>>> x=10
>>> print(x+5)
15
>>> exit()
piconix#
```

`python /tmp/test.py`はバナーを出さずにスクリプトを実行します．`cat > /tmp/test.py`でファイルを作成し，Ctrl+Dで入力を終えてください．カレントディレクトリが`/tmp`なら`python test.py`とも書けます．引数なしでは必ずREPLを起動し，既定のスクリプトファイルは読みません．

```python
n = int(input("Count: "))
i = 0
while i < n:
    if i % 2 == 0:
        print(i)
    i = i + 1
print("done")
```

## 対応する構文・機能

- 32ビット符号付き整数の変数（最大16個），四則演算・剰余（`+ - * / %`），括弧，比較演算．
- 代入，`if`／`else`，`while`，`#`コメント，空白インデントによるブロック．
- `print(整数式)`，`print("文字列リテラル")`，整数入力用の`input("プロンプト")`と`int(input(...))`．
- `sleep(ミリ秒)`（0～60000ミリ秒）．
- `gpio(pin, value)`，`gpio_read(pin)`，`gpio_mode(pin, mode)`，`gpio_pull(pin, mode)`．GPIOの操作には`/dev/gpio/N`，`/dev/gpio/N.dir`，`/dev/gpio/N.pull`を使用します．
- GPIOのモードは入力が0，出力が1です．プル設定は無効が0，プルアップが1，プルダウンが2です．GPIO 0，1，4，5と範囲外のピンは使用できません．
- ソースは最大512バイト，空行以外の64行まで，入れ子は8段までです．`while`の繰り返しは各文につき最大10000回です．

CPythonやMicroPythonとの互換性はありません．リスト，変数としての文字列，`for`，関数定義，`import`，浮動小数点などは実装されていません．入力はカーネルの行単位入力を使用します．`/tmp`のファイルは再起動で消去されます．

## 実行例と注意事項

```python
i = 0
while i < 5:
    gpio(25, 1)
    sleep(250)
    gpio(25, 0)
    sleep(250)
    i = i + 1
```

GPIO16をプルアップ入力に設定する場合は`gpio_mode(16,0)`，`gpio_pull(16,1)`を実行し，`gpio_read(16)`で読み取ります．GPIOピンから外部負荷を直接駆動しないでください．対話モードでは式を入力すると値が表示されますが，スクリプト中では`print(...)`を使用します．

## バックグラウンド実行

```sh
python /tmp/test.py &
python /tmp/test.py > /tmp/out.txt &
```

`python &`は対話入力がShellと競合するため使用できません．標準入力を必要とするバックグラウンドプログラムは，コンソールのフォアグラウンド制御によって入力を拒否される場合があります．
