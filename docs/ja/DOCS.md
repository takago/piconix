# PicoNIX ドキュメント案内


**[現行仕様書](SPECIFICATION.md)**：メモリ配置，リソース上限，ABI，ネットワーク構成の開発者向け参照資料．

[プロジェクト概要](../../README.md) | [English](../en/README.md)

PicoNIXは，RP2040上で動く小型UNIX風OSです．初めて使う方は，**準備 → シェル操作 → OSの仕組み → プログラミング**の順に読むと理解しやすくなります．

## 1．まず動かす

1. [セットアップ](SETUP.md)：開発環境の準備，ビルド，書き込み
2. [概要と最初のコマンド](README.md)：`help`，`ls`，`cat`，`ps`など
3. [デバイスと配線](DEVICES.md)：GPIO，PWM，ADC，LEDマトリクス
4. [ファイルシステムとノード一覧](FILESYSTEM.md)：ディレクトリ構造，`/dev`，`/proc`，RAMファイルの制約

## 2．UNIXの操作を学ぶ

| 学びたい内容 | 説明 |
|---|---|
| [パイプと標準入出力](PIPE.md) | `|`，リダイレクト，FD，EOF，制限 |
| [TTYと入力モード](TTY.md) | `stty`，canonical/raw，`fcntl()`，`O_NONBLOCK` |
| [端末の画面制御](TERMINAL.md) | ANSI制御，Ctrl+C，画面復帰 |
| [USB・UART端末](UART.md) | `getty`，UART1，端末の使い分け |
| [プロセスの観察](TRACE.md) | `trace`による実行追跡 |
| [メモリの観察](EDIT-MEM.md) | `edit`と`mem`の操作 |

## 3．ネットワークを使う（Pico W）

1. [Wi-Fi接続](WIFI-MODES.md)：AP／STAモード
2. [ネットワークの使い方](NETWORK.md)：接続とサービス
3. [ソケットプログラミング](NETWORK-PROGRAMMING.md)：TCP／UDPのユーザアプリ
4. [nc](NC.md)，[httpd](HTTPD.md)，[UDPデバイス制御](UDP-DEVICES.md)，[GPIO遠隔制御](REMOTE_GPIO.md)：具体例

## 4．プログラムを作る

| 内容 | 説明 |
|---|---|
| [Cアプリ開発](PROGRAMMING.md) | コマンド追加，ビルド，実行 |
| [システムコール一覧](SYSCALLS.md) | SVC呼び出し規約と番号 |
| [簡易Thumbアセンブラ](ASM.md) | `as`，`svc #0`，命令と制限 |
| [Python風インタプリタ](PYTHON.md) | `python`の機能と使い方 |
| [内部構造](INTERNAL.md) | タスク，メモリ，カーネル |

## 開発履歴

過去の設計・実装・障害調査の記録は[開発履歴](../history/README.md)にまとめています．現行仕様とは区別してください．
