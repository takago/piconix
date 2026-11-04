# ハードウェア — 技術リファレンス

PicoNIXを動かす**基板と実際の接続**について調べる入口です．OSのファイル操作は[デバイスリファレンス](../devices/DEVICES.md)，ドライバを作る方法は[ドライバ開発ガイド](../../development/drivers/DRIVER-DEVELOPMENT.md)に分けています．

| 調べたいこと | 文書 |
|---|---|
| 対応ボード，前提となる構成，安全な始め方 | [ハードウェア概要](HARDWARE.md) |
| 40ピン端子配置，GPnの用途，予約ピン，設定の変更 | [ピン割当とボード端子](PINS.md) |
| LEDマトリクス，ボタン，PWM，UART，I²Cなどの接続 | [配線と電気的注意](WIRING.md) |

**推奨順序：** [概要](HARDWARE.md) → [ピン割当](PINS.md) → [配線](WIRING.md) → [デバイスの操作](../devices/DEVICES.md)．

実装との対応は`kernel/pins.h`（ピンの用途）と`kernel/fb.c`（LED走査），使用する実部品のデータシートで確認してください．

---

**関連リンク：** [リファレンス総目次](../README.md) ｜ [デバイス・I/O](../devices/README.md) ｜ [教科書](../../learn/README.md) ｜ [English](../../../en/reference/hardware/README.md)
