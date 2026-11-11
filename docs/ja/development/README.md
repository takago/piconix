# 開発ガイド一覧
**前提知識：** C言語，UNIXの基本操作，OSのプロセス・FD・システムコールを想定します．この章は一般理論の再説明ではなく，現行ソースの読解と変更手順に集中します．変更前に[Linuxとの差異](../reference/DIFFERENCES.md)と[メモリ配置](../reference/MEMORY-LAYOUT.md)を確認してください．


これらは現行実装に対応した**API・内部構造・開発手順・実装例**です．必要な機能の資料を直接参照できます．

**目的から探す：** [やりたいこと → 関連資料](../TASKS.md)．

## Cプログラミングとユーザ空間

- [PicoNIXのCアプリケーション開発](apps/c/PROGRAMMING.md)
- [PicoNIXのCライブラリとユーザアプリケーションAPI](apps/c/LIBC.md)
- [ソケットAPIとネットワークアプリケーション](apps/c/NETWORK-PROGRAMMING.md)
- [MCPサーバ`mcpd`の内部構造と拡張](apps/c/MCP-DEVELOPMENT.md)

## Pythonとアセンブラ

- [PicoNIX Tiny Python — 利用ガイド](apps/python/PYTHON.md)
- [Tiny Pythonによるデバイスプログラミング](apps/python/DEVICE-PROGRAMMING.md)
- [Tiny PythonによるTCP/UDPソケットプログラミング（Pico W）](apps/python/SOCKET-PROGRAMMING.md)
- [PicoNIX 簡易Thumbアセンブラ](apps/asm/ASM.md)

## カーネルのソース索引と拡張

- [PicoNIX カーネルソース・処理経路索引](kernel/KERNEL-READING-GUIDE.md)
- [カーネル拡張ガイド](kernel/KERNEL-DEVELOPMENT.md)

## システムコール

- [システムコール開発ガイド](syscalls/SYSCALL-DEVELOPMENT.md)

## デバイスドライバ

- [デバイスドライバ開発ガイド](drivers/DRIVER-DEVELOPMENT.md)

## ネットワークのカーネル実装（Pico W）

- [ネットワーク開発ガイド目次](network/README.md)
- [Wi-Fi・lwIP・ソケット・カーネル常駐サービスの実装](network/NETWORK-DEVELOPMENT.md)

**ソース索引：** [関数一覧（日本語）](FUNCTIONS.md)．番号や行位置はコード変更で変わり得ます．

**実装例と拡張手順：** [CでUDP/TCPクライアントを作る](apps/c/NETWORK-PROGRAMMING.md) ／ [新しいシステムコールを追加する](syscalls/SYSCALL-DEVELOPMENT.md) ／ [新しいデバイスノードとドライバを作る](drivers/DRIVER-DEVELOPMENT.md)．いずれも準備，ソース，ビルド，実行，確認まで記載しています．

---

**関連リンク：** [English](../../en/development/README.md)
