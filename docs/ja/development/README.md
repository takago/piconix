# 開発ガイド一覧

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
