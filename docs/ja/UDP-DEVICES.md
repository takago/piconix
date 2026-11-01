# UDPデバイスサービス（Pico W）

Pico WはUDPポート2000で，限定されたデバイスファイルへの読み書きを提供します．APモードではPicoNIXのWi-Fiに接続し，STAモードではPico Wと同じ到達可能なネットワークから利用します．以下の`192.168.4.1`はAPモードの例です．STAモードではPico Wの割り当てアドレスに置き換えてください．

## netcatによる単発操作

```sh
printf 'READ /dev/adc/0' | nc -u -w 1 192.168.4.1 2000
printf 'WRITE /dev/pwm/1/freq\n50' | nc -u -w 1 192.168.4.1 2000
printf 'WRITE /dev/pwm/1/a\n7.5' | nc -u -w 1 192.168.4.1 2000
printf 'ENABLE /dev/gpio/fb' | nc -u -w 1 192.168.4.1 2000
printf 'WRITE /dev/gpio/fb\n\x18\x3c\x7e\xff\xff\x7e\x3c\x18' | nc -u -w 1 192.168.4.1 2000
printf 'DISABLE /dev/gpio/fb' | nc -u -w 1 192.168.4.1 2000
```

`WRITE`ではパスとデータを改行（`\n`）で区切ります．応答には`OK\n`が付くため，バイナリのFB読み取りには`host/piconixctl.py`などを利用してください．`nc`の終了条件は実装によって異なります．

## Pythonによる連続操作

`host/demo_udp.py`は8×8 LEDの点灯位置を移動させながら，ADC0を繰り返し読み取ります．Ctrl+Cで終了し，終了時にFBの無効化を試みます．

```sh
python3 host/demo_udp.py --interval 0.2
```

単発操作用の`host/piconixctl.py`も引き続き利用できます．Python 3標準ライブラリのみを使用し，再送は行いません．

## プロトコル

1要求・1応答，UDP 2000番．要求は`READ <path>`，`WRITE <path>\n<bytes>`，`ENABLE <path>`，`DISABLE <path>`．成功応答は`OK\n`に続く生バイト列，失敗応答は`ERR`です．要求の上限は128バイトです．FBは8バイトのバイナリ，PWMはASCII数値，ADCはASCII数値です．

対応パスは`/dev/gpio/fb`，`/dev/pwm/1/freq`，`/dev/pwm/1/a`，`/dev/pwm/1/b`，`/dev/adc/0`～`2`，`/dev/adc/temp`および，利用可能な`/dev/gpio/<番号>`です．GPIO操作は[遠隔GPIO制御](REMOTE_GPIO.md)に記載したピン制約を受けます．FBの有効化・無効化と許可されたGPIOピンの入出力切替には`ENABLE`・`DISABLE`を使用します．

再送制御，要求ID，認証はありません．パケットが失われた場合はクライアントがタイムアウトします．信頼できるローカルネットワークでのみ使用してください．移動体のモーターを制御する場合は，通信断時に停止する独立した安全機構が必要です．

ADCはローカルのファイル操作でも最初の`read()`でサンプリングし，その後はEOFまで読み進めます．再測定には再オープンが必要です．
