# PicoNIX Tiny Python — 利用ガイド

[English](../en/PYTHON.md) | [デバイス](DEVICES.md) | [I²C](I2C.md)

PicoNIXの小型Python風インタプリタです．CPythonやMicroPythonとは異なり，標準Pythonとの完全な互換性はありません．本書は現在の`userland/python.c`を基準とした利用ガイドです．

## 起動とスクリプト

`python`でREPL，`python /tmp/test.py`でスクリプトを実行します．REPLの終了は`exit()`です．`if`，`while`，`for ... in range(...)`に対応します．ブロックはスペースでインデントし，REPLでは空行で確定します．スクリプトとREPLの両方で，括弧を閉じるまで複数行のリストや`bytes([...])`を入力できます．

## 整数と配列

- 整数は符号付き32ビットです．`123`，`0x7B`，`0b01111011`，`-0x20`が使えます．`0X`と`0B`も認識します．
- `a = [1, 2, 3]`は変更可能な整数リストです．`a[0]`と`a[1] = 5`を利用できます．
- `b = bytes([0x01, 0xFF])`は変更不可のバイト列です．要素は0〜255です．
- `len(a)`，`len(b)`，`print(a)`，`print(b)`を利用できます．各配列は最大64要素です．
- `/`と`//`はどちらも整数除算です．浮動小数点，辞書，スライス，`append()`，`import`，`def`，例外処理は未対応です．

## ファイルディスクリプタAPI

| 関数 | 動作 |
|---|---|
| `open("/path", flags)` | FDを整数で返す．0=読取，1=書込，2=読書き（この環境のフラグ値） |
| `read(fd, n)` | 最大64バイトを読み，変更不可のバイト列を返す．代入文で使用 |
| `write(fd, data)` | 文字列リテラルまたは配列変数を書き込む．書込バイト数を返す |
| `close(fd)` | FDを閉じる |
| `ioctl(fd, cmd, arg)` | 整数引数のioctlを実行．ポインタ引数の構造体を直接構築する機能はない |

`open()`のパスは文字列リテラルで指定します．`write()`に渡す文字列もリテラルに限定されます．`read()`は`data = read(fd, 8)`の形で使います．失敗時は構文エラーと実行時エラーをまとめて`python: syntax/runtime error at statement N`のように表示します．`N`は物理行番号ではなく，空行・コメントを除いて継続行を結合した文番号です．

### LEDマトリクス

```python
pattern = bytes([0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c])
fd = open("/dev/gpio/fb", 2)
ioctl(fd, FB_ENABLE, 0)
write(fd, pattern)
close(fd)
```

フレームバッファは8バイト単位で書き込みます．`FB_ENABLE`で表示を有効化し，`FB_DISABLE`で無効化してGPIOを解放します．`FB_STATUS`で状態を取得できます．`close()`後も表示状態は維持されます．

### PWM

```python
fd = open("/dev/pwm/1/freq", 1)
write(fd, "440")
close(fd)
fd = open("/dev/pwm/1/a", 1)
write(fd, "50")
close(fd)
```

PWM周波数は8〜100000 Hz，Dutyは0〜100%の整数テキストです．

## I²C1

`i2c_write(addr, sequence)`，`data = i2c_read(addr, count)`，`data = i2c_xfer(addr, sequence, count)`を提供します．アドレスは0x08〜0x77，送受信は各最大32バイトです．`i2c_xfer()`はRepeated STARTを使用します．`i2c_write(0x23, bytes([0x10]))`のような直接指定にも対応します．I²Cの構造体転送には専用関数を使ってください．

### BH1750（アドレス0x23）

```python
power = bytes([0x01])
i2c_write(0x23, power)
msleep(100)
mode = bytes([0x10])
i2c_write(0x23, mode)
msleep(200)
for i in range(10):
    data = i2c_read(0x23, 2)
    raw = data[0] * 256 + data[1]
    lux1000 = raw * 1000 * 5 / 6
    print(lux1000)
    msleep(200)
off = bytes([0x00])
i2c_write(0x23, off)
```

表示値は**1000倍した照度値**です（例：125833は約125.833 lx）．32ビット整数の範囲に注意してください．

## 制約と検証

- ソースバッファ1024バイト，最大64の有効行，変数スロット16，配列長64です．REPLの1行入力バッファは128バイトです．
- 配列は内部的に1要素あたり32ビットを使用します．`bytes`もコンパクトな1バイト配列ではありません．
- `print()`は1引数です．`input()`は整数入力を返します．`msleep()`は0〜60000msです．
- I²CによるBH1750の照度測定，フレームバッファへの書き込みと`FB_ENABLE`，スクリプト・REPLでの複数行`bytes([...])`は実機で動作を確認しています．全機能の網羅的な検証を意味するものではありません．
- GPIO専用の旧Python関数ではなく，`/dev/gpio/<pin>`，`/dev/gpio/<pin>.dir`，`/dev/gpio/<pin>.pull`と共通のファイルAPIを使用します．
