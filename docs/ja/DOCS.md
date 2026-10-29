# 文書案内

[概要](README.md) | [English](../en/README.md)

PicoNIXを初めて使う場合は，**セットアップ → READMEのコマンド例 → 興味のあるガイド**の順に読むと全体をつかみやすくなります．

| 目的 | 文書 |
|---|---|
| インストール，ビルド，書き込み | [セットアップ](SETUP.md) |
| OSの機能と制限を知る | [概要](README.md) |
| GPIO，PWM，ADC，UARTの配線 | [デバイスと配線](DEVICES.md) |
| USBとUARTの対話端末 | [UART1と端末](UART.md) |
| `stty`，`fcntl`，`mb`，`reset` | [端末設定と対話型入力](TTY.md) |
| LEDマトリクス・`fbctl` | [デバイスガイド（フレームバッファ）](DEVICES.md) |
| `python`を使う | [Python風インタプリタ](PYTHON.md) |
| 自作Cアプリを追加する | [Cアプリ開発](PROGRAMMING.md) |
| RAM配置，タスク，SVC，FDなど | [内部構造](INTERNAL.md) |
| トレースの記録と解析 | [トレース](TRACE.md) |

**備考**：`RESET.md`は既存リンクとの互換性のための案内ページです．端末設定の本文は`TTY.md`に統合しています．
