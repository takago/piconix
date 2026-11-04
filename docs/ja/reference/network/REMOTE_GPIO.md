# UDPによるGPIO遠隔制御

*技術リファレンス*

カーネルのUDPデバイス制御サービスは2000番ポートで要求を受け付けます．**認証や暗号化はありません**．信頼できるローカルネットワークでのみ使用してください．APモードの既定アドレスは`192.168.4.1`です．STAモードではDHCPで割り当てられたPico Wのアドレスに置き換えてください．

Linuxからの例：

```sh
printf 'READ /dev/gpio/16\n' | nc -u -w 2 192.168.4.1 2000
printf 'ENABLE /dev/gpio/22\n' | nc -u -w 2 192.168.4.1 2000
printf 'WRITE /dev/gpio/22 1\n' | nc -u -w 2 192.168.4.1 2000
printf 'WRITE /dev/gpio/22 0\n' | nc -u -w 2 192.168.4.1 2000
printf 'DISABLE /dev/gpio/22\n' | nc -u -w 2 192.168.4.1 2000
```

`ENABLE`は出力，`DISABLE`は入力へ切り替えます．`READ`の応答は`OK\n0\n`または`OK\n1\n`，書き込み成功時は`OK\n`です．GPIO 0〜17および22は，ピンの予約状態が許す場合に操作できます．LEDフレームバッファ使用中のGPIO 0〜15，PWM用18/19，UART用20/21，Pico W用23〜25，ADC用26〜28は操作できません．Pico WのオンボードLEDはGP25ではなくCYW43が制御します．

実装は`gpio_driver_rw()`と`gpio_driver_ioctl()`に処理を委譲し，既存のデバイス制約を適用します．任意のVFSパスを操作できる仕組みではありません．[UDPデバイスサービス](REMOTE-DEVICE.md)も参照してください．

---

**関連リンク：** [リファレンス目次](../README.md) ｜ [学習ロードマップ](../../ROADMAP.md) ｜ [English](../../../en/reference/network/REMOTE_GPIO.md)
