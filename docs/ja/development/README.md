# 開発ガイド一覧
**前提：** C言語，UNIXの基本操作，OSのプロセス・FD・システムコールを既習とします．この章は一般理論の再説明ではなく，現行ソースの読解と変更手順に集中します．変更前に[Linuxとの差異](../reference/DIFFERENCES.md)と[メモリ配置](../reference/MEMORY-LAYOUT.md)を確認してください．


これらは実装・改造のための**詳しい開発手順とAPI説明**です．Part Cの学習順序は[学習ロードマップ](../ROADMAP.md)を基準とし，必要な箇所だけ開いてください．

**目的から探す：** [やりたいこと → 関連資料](../TASKS.md)．

## Cプログラミングとユーザ空間

- [PicoNIXのCアプリケーション開発](apps/c/PROGRAMMING.md)
- [PicoNIXのCライブラリとユーザアプリケーションAPI](apps/c/LIBC.md)
- [ソケットAPIとネットワークアプリケーション](apps/c/NETWORK-PROGRAMMING.md)
- [MCPサーバ`mcpd`の開発実習](apps/c/MCP-SERVER-DEVELOPMENT.md)

## Pythonとアセンブラ

- [PicoNIX Tiny Python — 利用ガイド](apps/python/PYTHON.md)
- [Tiny Pythonによるデバイスプログラミング](apps/python/DEVICE-PROGRAMMING.md)
- [Tiny PythonによるTCP/UDPソケットプログラミング（Pico W）](apps/python/SOCKET-PROGRAMMING.md)
- [PicoNIX 簡易Thumbアセンブラ](apps/asm/ASM.md)

## ビルド時のROM/RAM使用量

`tools/build_sdk.sh`は最終リンク後に`tools/size_ram.py`を呼び出します．単独で再表示する場合は，例えば`python3 tools/size_ram.py --build build-pico_w`を実行します．

`Firmware total`はELFに割り当てられたFlashとSRAM全体の集計です．`Link-map attribution`は，Flash/SRAMの内訳を`PicoNIX`（カーネル固有コードなど），`Embedded apps`（組込みユーザアプリ），`ROM manual`（XIPの`.manual`セクション），`Symbol catalogs`，`Pico SDK`，`lwIP`，`CYW43`，`Other`に分類します．`ROM manual`には索引，ページ本文，アラインメント領域が含まれ，SRAMを使用しません．これらの内訳を合算してさらに`Firmware total`へ加算してはいけません．未分類領域は`Unattributed / padding`として表示します．

分類はELFの割当て領域とリンクマップを使う推定値です．ROMマニュアルはELFの`.manual`から直接計上し，組込みアプリはELFシンボルのイメージ区間から計上します．`PicoNIX kernel component`の`Code`にはROMマニュアルと組込みアプリを含めません．マニュアルが欠落していれば警告します．集計の回帰テストは`python3 tools/test_scripts/test_rom_manual_size_report.py`です．

## カーネルの読解と拡張

- [PicoNIX カーネル読解ガイド](kernel/KERNEL-READING-GUIDE.md)
- [カーネル拡張ガイド](kernel/KERNEL-DEVELOPMENT.md)

## システムコール

- [システムコール開発ガイド](syscalls/SYSCALL-DEVELOPMENT.md)

## デバイスドライバ

- [デバイスドライバ開発ガイド](drivers/DRIVER-DEVELOPMENT.md)

## ネットワークのカーネル実装（Pico W・選択）

- [ネットワーク開発ガイド目次](network/README.md)
- [Wi-Fi・lwIP・ソケット・カーネル常駐サービスの実装](network/NETWORK-DEVELOPMENT.md)

**ソース索引：** [関数一覧（日本語）](../../FUNCTIONS.md)．番号や行位置はコード変更で変わり得ます．

**実際にコードを変更する演習：** [CでUDP/TCPクライアントを作る](apps/c/NETWORK-PROGRAMMING.md) ／ [新しいシステムコールを追加する](syscalls/SYSCALL-DEVELOPMENT.md) ／ [新しいデバイスノードとドライバを作る](drivers/DRIVER-DEVELOPMENT.md)．いずれも準備，ソース，ビルド，実行，確認まで記載しています．

---

**関連リンク：** [学習ロードマップ](../ROADMAP.md) ｜ [English](../../en/development/README.md)
