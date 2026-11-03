# PicoNIX Tiny Python — 利用ガイド

## 読み始める前に

まず`python`の対話入力で計算と表示を試し，次に`/tmp`に短いスクリプトを保存して実行してください．Python標準の`import`や浮動小数点は使えません．


[English](../../../../en/development/apps/python/PYTHON.md) | [デバイス](../../../reference/devices/DEVICES.md) | [I²C](../../../reference/devices/I2C.md)

PicoNIXの小型Python風インタプリタです．CPythonやMicroPythonとは異なり，標準Pythonとの完全な互換性はありません．本書は現在の`userland/apps/python.c`を基準とした利用ガイドです．

## 初めて使う場合

まず`python`で対話入力を始め，`print(1 << 3)`や`print("hello", 123)`を試します．スクリプトを書く場合は`edit /tmp/test.py`などでファイルを作り，`python /tmp/test.py`で実行します．**`python3`コマンドはありません**．

起動時に配置される`/tmp/test.py`は，内蔵温度センサー`/dev/adc/temp`を1秒間隔で5回読み取り，温度を表示するサンプルです．ネットワークは使用しません．`read()`の結果はバイト列なので，画面表示には`write(1, temp)`を使います．新しい測定値を得るため，毎回デバイスを開き直します．

GPIOの配線や8×8表示の座標は[デバイスガイド](../../../reference/devices/DEVICES.md)，BH1750の配線は[I²Cガイド](../../../reference/devices/I2C.md)を参照してください．以下のPythonは**標準Pythonではなく，実装機能が限定された小型インタプリタ**です．

## 起動とスクリプト

`python`でREPL，`python /tmp/test.py`でスクリプトを実行します．REPLの終了は`exit()`です．`if`，`while`，`for ... in range(...)`に対応します．ブロックはスペースでインデントし，REPLでは空行で確定します．スクリプトとREPLの両方で，括弧を閉じるまで複数行のリストや`bytes([...])`を入力できます．

## 整数，配列，文字列

- 整数は符号付き32ビットです．`123`，`0x7B`，`0b01111011`，`-0x20`が使えます．`0X`と`0B`も認識します．
- `a = [1, 2, 3]`は変更可能な整数リストです．`a[0]`と`a[1] = 5`を利用できます．
- `b = bytes([0x01, 0xFF])`は変更不可のバイト列です．要素は0〜255です．
- `len(a)`，`len(b)`，`print(a)`，`print(b)`を利用できます．各配列は最大64要素です．
- 配列と文字列は共有プール（256個の32ビット整数に相当する1024バイト）に格納します．1配列の上限は64要素，変数は整数を含め最大16個です．整数変数自体はプールを消費しません．文字列もプールを使用するため，配列を合計256要素分使えるのは文字列を保存していない場合です．プール不足時はエラーになります．
- `message = "hello"`のような文字列変数（最大64バイト），文字列変数同士の代入，`len(message)`，`print(message)`に対応します．文字列連結や文字列の添字参照は未対応です．
- `/`と`//`はどちらも整数除算です．浮動小数点，辞書，スライス，`append()`，`import`，`def`，例外処理は未対応です．

## ビット演算子

整数式では `<<`（左シフト），`>>`（右シフト），`&`（AND），`^`（XOR），`|`（OR），`~`（ビット反転）を使用できます．演算の優先順位は標準Pythonと同じく，単項演算，乗除算，加減算，シフト，AND，XOR，OR，比較の順です．シフト量は0〜31のみ許可します．値は符号付き32ビット整数で，左シフトは32ビット内で処理します．

```python
print(1 << 7, 0xF0 & 0x0F, 0x12 | 0x01)
fb = [0,0,0,0,0,0,0,0]
fb[0] = 1 << 3
fb[1] = fb[1] | (1 << 4)
```

## print()の複数引数

`print()`は0個以上の引数を受け取り，それぞれの値を空白で区切り，最後に改行します．整数式，文字列リテラル・文字列変数，配列変数，整数リストリテラルに対応します．

```python
message = "hello"
print([0x13, 0x12], message, 123)
# [19, 18] hello 123
print()
```

これは小型インタプリタのための限定的な文字列機能であり，Pythonの文字列演算全般をサポートするものではありません．

## ソケット通信（Pico W）

PicoNIX独自の整数FD型APIです．CPythonの`import socket`とは互換ではありません．IPv4の数値アドレスを使用します．ソケットはノンブロッキングです．

| API | 内容 |
|---|---|
| `socket(1)` / `socket(2)` | TCP / UDPソケットを作成 |
| `connect(fd, "127.0.0.1", 80)`，`status(fd)` | 非同期TCP接続と状態確認（0=処理中，1=成功） |
| `listen(fd, port)`，`accept(fd)` | TCP待受・接続受入（未到着=-11） |
| `bind(fd, port)` | UDP受信ポートを割り当て |
| `send(fd, data)`，`write(fd, data)` | TCP送信．部分送信に注意 |
| `data = recv(fd, n)` | TCP受信．最大64バイト．未到着とEOFはいずれも空配列 |
| `sendto(fd, data, "192.168.4.2", port)` | UDP送信 |
| `data = recvfrom(fd, n)` | UDP受信．最大64バイト，未到着は空配列 |
| `peerport()` | 直近の`recvfrom()`の送信元ポート |
| `sendto(fd, data, "peer", 0)` | 直近の`recvfrom()`の送信元へ返信（`peer`，0は専用表記） |

`data`には64バイト以下の文字列リテラル，文字列変数，`bytes`/リスト変数または`str(整数式)`を渡せます．文字列リテラルには`\n`，`\r`，`\t`，`\\`，`\"`，`\'`，`\xHH`を指定できます．文字列内のNULは通常の文字列変数では扱えませんが，送信リテラルの`\0`は1バイトとして送信します．

```python
fd = socket(1)
rc = connect(fd, "127.0.0.1", 80)
if rc == 0:
    state = 0
    while state == 0:
        state = status(fd)
        msleep(10)
    if state == 1:
        send(fd, "GET / HTTP/1.0\r\n\r\n")
        data = recv(fd, 64)
        print(data)
close(fd)
```

### 内蔵温度センサーのTCP・UDP配信サーバ

両サンプルはPico W用で，専用の温度関数を使わず，`/dev/adc/temp`を`open()`・`read()`で読みます．応答は摂氏温度のテキスト（例：`26.3\n`）です．センサーは要求ごとに開いて閉じます．RP2040の内蔵温度センサーはチップ温度の概算値であり，正確な室温ではありません．

| サンプル | 方式 | 動作 |
|---|---|---|
| `userland/examples/python/temp_tcp.py` | TCP 9000 | 接続を受けたら温度を1回送信して切断 |
| `userland/examples/python/temp_udp.py` | UDP 9000 | データグラムを受けたら送信元へ返信 |

Pico W側では，それぞれ`/tmp/temp_tcp.py`，`/tmp/temp_udp.py`にコピーまたは入力し，`python /tmp/temp_tcp.py`または`python /tmp/temp_udp.py`で実行します．

PC側の例：

```bash
# TCP: 接続すると1回分の温度を受信
nc <Pico WのIPアドレス> 9000

# UDP: 要求を送信して温度を受信
printf 'temp' | nc -u -w 2 <Pico WのIPアドレス> 9000
```

TCPとUDPは同じ9000番でも別のポート空間です．`accept()`と`recvfrom()`はノンブロッキングです．TCP `send()`には部分送信・一時的な送信不可があり得ますが，サンプルは短い応答を1度だけ送信する最小構成であり，再送処理はありません．UDPの`"peer"`，ポート`0`は直近の`recvfrom()`の送信元を指し，記憶する送信元は1件だけです．このPythonの`while`は1ループ最大10,000回で終了する制限があるため，恒久稼働を想定していません．外部公開せず実験用LANで利用してください．

## ファイルディスクリプタAPI

| 関数 | 動作 |
|---|---|
| `open("/path", flags)` | FDを整数で返す．0=読取，1=書込，2=読書き（この環境のフラグ値） |
| `read(fd, n)` | 最大64バイトを読み，変更不可のバイト列を返す．代入文で使用 |
| `write(fd, data)` | 文字列リテラルまたは配列変数を書き込む．書込バイト数を返す |
| `close(fd)` | FDを閉じる |
| `ioctl(fd, cmd, arg)` | 整数引数のioctlを実行．ポインタ引数の構造体を直接構築する機能はない |

`open()`のパスは文字列リテラルで指定します．`write()`に渡す文字列もリテラルに限定されます．`read()`は`data = read(fd, 8)`の形で使います．失敗時は構文エラーと実行時エラーをまとめて`python: syntax/runtime error at statement N`のように表示します．`N`は物理行番号ではなく，空行・コメントを除いて継続行を結合した文番号です．

### Pythonで表示を始める前に

最初にシェルで`fbctl test`を実行し，LEDが期待どおりに点灯することを確かめます．`/proc/fb`で確認できるのはフレームバッファのデータであって，物理配線の正しさではありません．ピン接続，電流制限，向きは[デバイスガイド](../../../reference/devices/DEVICES.md)を参照してください．

Pythonで`write(fd, pattern)`するときは，**各要素が1行分の8ビット**で，配列全体が8要素必要です．例えば`bytes([0x80,0,0,0,0,0,0,0])`はR0左端だけを指定します．フレームバッファの1は点灯を表しますが，カソードGPIOの出力はLOWになります．

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

- ソースバッファ2048バイト，最大64の有効行，変数スロット16，配列長64です．REPLの1行入力バッファは128バイトです．
- 配列は内部的に1要素あたり32ビットを使用します．`bytes`もコンパクトな1バイト配列ではありません．
- `print()`は複数引数に対応します．`input()`は文字列（最大64バイト）を返します．整数が必要な場合は`int(input())`を使用します．`msleep()`は0〜60000msです．
- I²CによるBH1750の照度測定，フレームバッファへの書き込みと`FB_ENABLE`，スクリプト・REPLでの複数行`bytes([...])`は実機で動作を確認しています．全機能の網羅的な検証を意味するものではありません．
- GPIO専用の旧Python関数ではなく，`/dev/gpio/<pin>`，`/dev/gpio/<pin>.dir`，`/dev/gpio/<pin>.pull`と共通のファイルAPIを使用します．

## 文字列入力と変換

`input()`は通常のPythonと同様に文字列を返します．引数なし，文字列リテラルまたは文字列変数によるプロンプトに対応します．`int()`は10進整数への変換，`str()`は整数から文字列への変換に対応します．`int()`の変換に失敗した場合は実行時エラーになります．

```python
name = input("Name: ")
age = int(input("Age: "))
print("Hello", name, age)
text = str(age)
print(text, len(name))
```

空行の入力は空文字列になります．文字列最大64バイト，変数16個の制限は従来どおりです．文字列連結や一般の文字列式は未対応です．従来の整数入力コード `n = input()` は `n = int(input())` に変更してください．

## サンプル：照度で速度が変わるLEDウェーブ

BH1750で測定した照度が高いほど，8×8 LEDマトリクス上の波が速く流れます．暗いとゆっくり動きます．I²C入力，整数演算，ビット演算，フレームバッファ出力を組み合わせたサンプルです．**実機で動作確認済み**です．

BH1750を3.3 VとGNDに接続し，SDAをGP26，SCLをGP27へ接続します．ADDRはGND側（I²Cアドレス`0x23`）にしてください．配線とI²Cの詳細は[I²Cガイド](../../../reference/devices/I2C.md)を参照してください．

サンプルのソースは[`userland/examples/python/wave.py`](../../../../../userland/examples/python/wave.py)にあります．`/tmp/wave.py`として初期配置されるため，`python /tmp/wave.py`で実行できます．ソース上限2048バイトとRAMファイル上限2 KiBに注意してください．

```python
# PicoNIX: light-controlled wave
wave = [3,4,5,6,5,4,3,2]
fb = [0,0,0,0,0,0,0,0]
fd = open("/dev/gpio/fb", 2)
ioctl(fd, FB_ENABLE, 0)
i2c_write(0x23, bytes([1]))
msleep(100)
i2c_write(0x23, bytes([0x10]))
msleep(200)

phase = 0
for n in range(10000):
    data = i2c_read(0x23, 2)
    lux = (data[0]*256+data[1])*5/6
    delay = 350 - lux/3
    if delay < 35:
        delay = 35
    for y in range(8):
        x = wave[(y+phase)%8]
        fb[y] = 1 << (7-x)
    write(fd, fb)
    phase = (phase+1)%8
    msleep(delay)

i2c_write(0x23, bytes([0]))
close(fd)
```

`wave`配列は各行の点灯位置，`phase`は波の位相を表します．照度から待機時間を `350 - lux/3` ミリ秒として計算し，最短35ミリ秒に制限します．約1万フレーム表示した後，BH1750をパワーダウンして終了します．この小型Pythonでは除算は整数除算です．外部センサが必要で，照度の数値を画面に表示するものではありません．

