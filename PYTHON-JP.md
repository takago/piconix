# Python風インタプリタ

[English](PYTHON-EN.md) | [概要へ戻る](README-JP.md)

`ram_apps/python.c`はPicoNIX独自の**小型Python風言語**です．CPythonやMicroPythonではありません．アプリケーションは起動時に空きRAMスロットへ配置されます．

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
| 待機 | `msleep(ミリ秒)`（0～60000） |
| GPIO | `gpio`，`gpio_read`，`gpio_mode`，`gpio_pull` |

変数は最大16個，ソースは512バイト／非空行64行，ブロックは8段までです．`while`は1文につき最大10000回の反復に制限されています．浮動小数点，リスト，ユーザ定義関数，`for`，`import`，文字列変数はありません．

## 4．GPIOの例

`/tmp/led.py`に次の内容を保存し，`python /tmp/led.py`で実行します．

```python
i = 0
while i < 5:
    gpio(25, 1)
    msleep(250)
    gpio(25, 0)
    msleep(250)
    i = i + 1
```

`gpio_mode(pin, 0)`が入力，`gpio_mode(pin, 1)`が出力です．`gpio_pull(pin, 0/1/2)`はそれぞれ無効／プルアップ／プルダウンです．UART用のGPIO 0，1，4，5は操作できません．ボードや接続回路の電気的制約にも注意してください．

## 5．バックグラウンド実行とUART1からの対話操作

```text
piconix# python /tmp/led.py &
piconix# python /tmp/led.py > /tmp/output.txt &
piconix# python < /dev/uart1 > /dev/uart1 &
```

最後の例では，USB接続のShellを操作したまま，UART1に接続した端末からPythonのREPLを使用できます．UART1はバイトストリームですが，Pythonが改行まで入力を蓄積します．UART端末ではローカルエコーを有効にすると入力文字を確認できます．改行はCR，LF，CRLFを処理します．

**注意：** `python &`のように入力をリダイレクトせずUSBコンソールを共有する使い方は避けてください．USBコンソールの入力所有権はフォアグラウンドタスクに制限されています．UART側ではCtrl+Cによるシグナル処理はありません．終了には`exit()`を使用してください．実機でのUART対話操作は未検証です．

## UART1での対話実行

`getty /dev/uart1 python &`でUART1にPythonを直接接続できます．標準入出力がUART1となり，Ctrl+Cはこの端末の前景タスクに作用します．対話モードではCtrl+Dを押しても終了せず，次の入力待ちに戻ります．`exit()`で終了すると，UART1側には`PicoNIX: Session closed.`が表示されます．スクリプト実行時のEOF処理は変更しません．
