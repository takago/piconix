# Tiny Pythonによるデバイスプログラミング

*開発ガイド — Raspberry Pi Pico / Pico W共通*

[Pythonの基本](PYTHON.md) | [ソケットプログラミング](SOCKET-PROGRAMMING.md) | [デバイスリファレンス](../../../reference/devices/DEVICES.md) | [配線](../../../reference/hardware/WIRING.md) | [English](../../../../en/development/apps/python/DEVICE-PROGRAMMING.md)

PicoNIXのデバイスは`/dev`のファイルとして公開されます．Python専用のGPIOライブラリは不要で，`open()`，`read()`，`write()`，`close()`で操作します．本書は`userland/apps/python.c`と現行のデバイス文書を基準にしています．

## 実習の準備

シェルで`ls /dev`，`ls /dev/gpio`，`ls /dev/adc`を確認してください．`edit /tmp/demo.py`でスクリプトを作り，`python /tmp/demo.py`で実行します．Pythonの標準的な`import`，`def`，`try`は使えません．ファイル名は`/tmp`の容量・名前長の制約を受けます．

### FD操作の早見表

| 書式 | 動作・注意 |
|---|---|
| `fd = open("/dev/...", 0)` | 読取り専用．フラグ0=読取，1=書込，2=読書き |
| `data = read(fd, 32)` | 最大64バイトの`bytes`相当を取得．**代入文が必要** |
| `n = write(fd, "123")` | テキストやバイト列を送信．実際の書込バイト数を返す |
| `close(fd)` | FDを閉じる |
| `value = ioctl(fd, FB_STATUS, 0)` | 整数引数のデバイス制御 |
| `pos = lseek(fd, 0, SEEK_SET)` | シーク可能なFDのみ．`SEEK_SET`，`SEEK_CUR`，`SEEK_END`に対応 |

`open()`のパスは文字列**リテラル**に限定されます．`read()`と`write()`は1回最大64バイトです．`write()`に渡せるのは文字列リテラル・文字列変数・整数リスト・`bytes`変数・`str(整数式)`などです．データはバイト列であり，通常のPythonのファイルオブジェクトや`decode()`はありません．`read()`の結果を`print()`するとバイト値の配列表示になるため，テキストを表示するには`write(1, data)`が便利です．エラーは操作により負の整数が返る場合と，インタプリタの実行時エラーで停止する場合があります．

## 1．ADC・内蔵温度センサー

まずシェルで`cat /dev/adc/temp`を確認します．次を`/tmp/temp.py`に入力してください．

```python
for i in range(5):
    sensor = open("/dev/adc/temp", 0)
    data = read(sensor, 32)
    close(sensor)
    write(1, data)
    msleep(1000)
```

`python /tmp/temp.py`で約1秒間隔に5回表示します．内蔵温度はRP2040チップ温度の概算です．新しい測定値を取得するために毎回デバイスを開き直します．`/tmp/test.py`にも同じ目的の初期サンプルがあります．外部ADCはボードのピン設定に依存し，標準構成では`/dev/adc/2`が対象です．

## 2．GPIOとLED

まず`ls /dev/gpio`で利用可能なピンを確認します．GP25を使う場合，次を`/tmp/led.py`に保存します．

```python
fd = open("/dev/gpio/25", 1)
for i in range(5):
    write(fd, "1")
    msleep(300)
    write(fd, "0")
    msleep(300)
close(fd)
```

この例はGP25を出力として利用できる標準構成を前提とします．ボード実装によってはGP25がユーザ可視LEDと直結していないため，`/dev/gpio/25`への書込みとLED点灯が必ず一致するとは限りません．GPIOの方向やプル設定は`/dev/gpio/<pin>.dir`，`/dev/gpio/<pin>.pull`で管理します．[ピン割当](../../../reference/hardware/PINS.md)と[GPIOリファレンス](../../../reference/devices/DEVICES.md)を確認してください．

## 3．PWM

ブザーなどの接続を確認してから実行します．PWMスライス1のAはGP18，BはGP19で，共通周波数を使用します．

```python
fd = open("/dev/pwm/1/freq", 1)
write(fd, "440")
close(fd)
duty = open("/dev/pwm/1/a", 1)
write(duty, "50")
msleep(500)
write(duty, "0")
close(duty)
```

周波数は8～100000 Hz，デューティ比は0～100%です．使用するハードウェアの安全な接続方法を必ず確認してください．

## 4．8×8 LEDフレームバッファ

最初にシェルで`fbctl test`を実行し，配線と表示方向を確認します．次の1バイトが1行で，8行分をまとめて書き込みます．

```python
pattern = bytes([0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c])
fd = open("/dev/gpio/fb", 2)
ioctl(fd, FB_ENABLE, 0)
write(fd, pattern)
close(fd)
```

1は点灯，0は消灯です．`close()`しても表示は残るので，終了時にシェルで`fbctl off`を実行するとGPIOを解放できます．`FB_STATUS`，`FB_DISABLE`も利用できます．

## 5．I²C1とBH1750

GP26がSDA，GP27がSCLです．BH1750のADDRをGNDへ接続すると通常は`0x23`です．配線を確認し，シェルで`i2cctl scan`を実行してから試します．

```python
i2c_write(0x23, bytes([0x01]))
msleep(100)
i2c_write(0x23, bytes([0x10]))
msleep(200)
data = i2c_read(0x23, 2)
raw = data[0] * 256 + data[1]
print(raw * 5 / 6)
i2c_write(0x23, bytes([0x00]))
```

これは照度の整数近似値を表示します．I²Cアドレスは`0x08`～`0x77`，送受信は各32バイトまでです．書込みと読取りをRepeated STARTでまとめる場合は`i2c_xfer(addr, sequence, count)`を使用します．詳細は[I²Cリファレンス](../../../reference/devices/I2C.md)を参照してください．

## 6．実行時の注意

- スクリプト上限は2048バイト，有効行は最大64，変数は最大16個です．配列・受信バイト列は各64要素までです．
- `open()`や`ioctl()`が失敗するとPythonの実行が中断される場合があります．実習では先に`ls`，`cat`，`fbctl`などでデバイスの有無を確認してください．
- 長時間のデバイス制御では，終了前にPWM出力を0にするなど，安全な状態へ戻してください．
- `/dev/mem`や`/dev/trace`のような特殊なバイナリデバイスは，一般的な文字列の読書きとは制約が異なります．仕様を確認してから利用してください．
- Pythonからデバイスを直接操作する場合も，GPIOとPWM・ADC・I²Cのピンの競合は解消されません．

**次の実習：** [Pico WでのTCP/UDP通信](SOCKET-PROGRAMMING.md) ／ [Python基本ガイド](PYTHON.md)
