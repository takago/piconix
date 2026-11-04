# 技術リファレンス — 目次

OSの仕様，API，機能の制限などを分野別に調べられます．順番にUNIXを学ぶ場合は[学習ロードマップ](../ROADMAP.md)を参照してください．

**目的から探す：** [やりたいこと → 関連資料](../TASKS.md)．

## OSの基本構造と仕様

- [PicoNIX 全体アーキテクチャ](ARCHITECTURE.md)
- [PicoNIXとLinuxの違い](DIFFERENCES.md)
- [ファイルシステムとデバイスノード](FILESYSTEM.md)
- [初期RAMファイルと /etc/rc](INITIAL-FILES.md)
- [PicoNIXの内部構造](INTERNAL.md)
- [`mem` のカーネルシンボル表示（現行実装）](KERNEL-SYMBOLS.md)
- [RP2040 SRAMレイアウト（現行配置）](MEMORY-LAYOUT.md)
- [PicoNIX 仕様書](SPECIFICATION.md)
- [システムコール・ABIリファレンス](SYSCALLS.md)

## デバイス・I/O

- [デバイス・I/O リファレンス目次](devices/README.md)
- [GPIO・ADC・PWM・フレームバッファ](devices/DEVICES.md)
- [I²C1とi2cctl](devices/I2C.md)
- [UART1と端末操作](devices/UART.md)

## ネットワーク（Pico W）

- [ネットワーク・リファレンス目次](network/README.md)
- [Pico Wネットワーク接続](network/NETWORK.md)
- [Wi-Fiモードと状態表示](network/WIFI-MODES.md)
- [ネットワーク診断](network/NET.md)
- [nc（netcat）](network/NC.md)
- [HTTPサーバ](network/HTTPD.md)
- [リモートデバイス制御（UDP 2000番）](network/REMOTE-DEVICE.md)
- [UDPによるGPIO遠隔制御](network/REMOTE_GPIO.md)

## ハードウェア

- [ハードウェア目次](hardware/README.md)
- [ハードウェア概要](hardware/HARDWARE.md)
- [ピン割当と40ピン配置図](hardware/PINS.md)
- [配線・電気的注意](hardware/WIRING.md)

[学習ロードマップ](../ROADMAP.md) ｜ [補助教材一覧](../guides/README.md) ｜ [開発ガイド一覧](../development/README.md)

---

**関連リンク：** [文書案内](../../DOCS.ja.md) ｜ [English](../../en/reference/README.md)
