# デバイスと入出力

[学習ガイド](LEARNING-PATH.md) | [English](../../en/literacy/DEVICE-IO.md) | [デバイス仕様](../reference/devices/DEVICES.md)

## 目的と準備

**目的：** `/dev`を通して周辺機器をファイルと似たインターフェースで扱える理由を理解することです．前半はUSB接続だけで行えます．GPIO，I2C，UART，LEDマトリクスの実験には対応する配線と安全確認が必要です．[ハードウェア仕様](../reference/hardware/HARDWARE.md)を先に参照してください．

## 実習A：デバイスを探す（外付け回路不要）

```text
piconix# ls /dev
piconix# ls /dev/gpio
piconix# cat /proc/fb
piconix# tty
```

`/proc/fb`はLEDの表示データをテキストで調べるための入口です．**表示データが正しくても実際の配線が正しいとは限りません**．端末に出力する操作と，物理デバイスを制御する操作は区別しましょう．

## 実習B：接続したデバイスを操作する（任意）

接続済みのLEDマトリクスがある場合は[フレームバッファの操作](../reference/devices/DEVICES.md)を参照し，`fbctl`の使用方法を確認してから試します．GPIO，I2C，UARTもそれぞれ[デバイスガイド](../reference/devices/DEVICES.md)，[I2C](../reference/devices/I2C.md)，[UART](../reference/devices/UART.md)に従います．**未配線のまま出力コマンドを試したり，3.3 Vを超える電圧を入力したりしないでください．**

## 観察・考察

- `/tmp`のファイルと`/dev`のファイルは，読み書きの結果がどう違いますか．
- `read()`と`write()`に加えて`ioctl()`が必要になるのはどんな操作でしょうか．
- 配線がないときでもソフトウェア側で確認できる状態は何でしょうか．

## 発展

[Cによるアプリ開発](../development/apps/c/PROGRAMMING.md)でデバイスへの読み書きを学び，[ドライバ開発](../development/drivers/DRIVER-DEVELOPMENT.md)で新しいノードを追加する方法へ進めます．
