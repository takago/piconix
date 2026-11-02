# PicoNIX ドキュメント案内


**[現行仕様書](SPECIFICATION.md)**：メモリ配置，リソース上限，ABI，ネットワーク構成の開発者向け参照資料．

[プロジェクト概要](../../README.md) | [English](../en/DOCS.md)

PicoNIXは，RP2040上で動く小型UNIX風OSです．初めて使う方は，**準備 → シェル操作 → OSの仕組み → プログラミング**の順に読むと理解しやすくなります．

## 1．まず動かす

1. [セットアップ](SETUP.md)：開発環境の準備，ビルド，書き込み
2. [概要と最初のコマンド](README.md)：`help`，`ls`，`cat`，`ps`など
3. [デバイスと配線](DEVICES.md)：GPIO，PWM，ADC，I²C，LEDマトリクス
4. [ファイルシステムとノード一覧](FILESYSTEM.md)：ディレクトリ構造，`/dev`，`/proc`，RAMファイルの制約

[初期RAMファイルと起動スクリプト](INITIAL-FILES.md)：ホスト側のファイル配置，ビルド時登録，`/etc/rc`とPID 1のシェル．

## 2．UNIXの操作を学ぶ

| 学びたい内容 | 説明 |
|---|---|
| [パイプと標準入出力](PIPE.md) | `|`，リダイレクト，FD，EOF，制限 |
| [TTYと入力モード](TTY.md) | `stty`，canonical/raw，`fcntl()`，`O_NONBLOCK` |
| [端末の画面制御](TERMINAL.md) | ANSI制御，Ctrl+C，画面復帰 |
| [USB・UART端末](UART.md) | `getty`，UART1，端末の使い分け |
| [プロセスの観察](TRACE.md) | `trace`による実行追跡 |
| [メモリの観察](MEM.md) | `mem`による読み取り専用のメモリ参照 |

## 3．ネットワークを使う（Pico W）

1. [Wi-Fi接続](WIFI-MODES.md)：AP／STAモード
2. [ネットワークの使い方](NETWORK.md)：接続とサービス
3. [ソケットプログラミング](NETWORK-PROGRAMMING.md)：TCP／UDPのユーザアプリ
4. [nc](NC.md)，[httpd](HTTPD.md)，[UDPデバイス制御](REMOTE-DEVICE.md)，[GPIO遠隔制御](REMOTE_GPIO.md)：具体例

[現行SRAM配置・共有メモリ・lwIPヒープ](MEMORY-LAYOUT.md)：固定アドレス，`/dev/shm`，ネットワーク診断とアセンブリ例．

**[全体アーキテクチャ](ARCHITECTURE.md)**：ユーザ空間とカーネルの分担，ソースコードの対応表．

[I²C1とi2cctl](I2C.md)：配線，コマンド，C・Pythonからの通信．

## 4．プログラムを作る

| 内容 | 説明 |
|---|---|
| [テキストエディタ](EDIT.md) | `edit`によるRAMファイルの編集 |
| [viエディタ](VI.md) | `vi`によるモード型テキスト編集 |
| [ddとバイナリ転送](DD.md) | ファイルコピー，RAM読み書き，nc転送 |
| [Cアプリ開発](PROGRAMMING.md) | コマンド追加，ビルド，実行 |
| [Cライブラリ・API](LIBC.md) | 提供されるヘッダ，関数，引数，戻り値と制限 |
| [システムコール一覧](SYSCALLS.md) | SVC呼び出し規約，番号，引数，戻り値 |
| [簡易Thumbアセンブラ](ASM.md) | `as`，`svc #0`，命令と制限．ABIは[システムコール](SYSCALLS.md)を参照 |
| [Python風インタプリタ](PYTHON.md) | `python`の機能と使い方 |
| [内部構造](INTERNAL.md) | タスク，メモリ，カーネル |

Cアプリを作る場合は **Cアプリ開発 → Cライブラリ・API → システムコール** の順で参照すると便利です．アセンブリを使う場合は **簡易Thumbアセンブラ → システムコール** の順に確認してください．

## 開発履歴

過去の設計・実装・障害調査の記録は[開発履歴](../history/README.md)にまとめています．現行仕様とは区別してください．
