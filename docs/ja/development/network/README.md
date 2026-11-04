# ネットワーク開発ガイド

**資料区分：開発ガイド目次（Pico W・カーネル側）** ｜ [開発ガイド一覧](../README.md) ｜ [技術リファレンス](../../reference/network/README.md) ｜ [English](../../../en/development/network/README.md)

ここは，PicoNIXの**カーネルにあるネットワーク処理を読んだり，変更したりする人**の入口です．UNIXの学習順序は[教科書C5](../../learn/C/C5.md)，実際の接続・コマンドの仕様は[ネットワークリファレンス](../../reference/network/README.md)を参照してください．

## 目的別の入口

| やりたいこと | 読む資料 |
|---|---|
| カーネル側のWi-Fi・lwIP・ソケットを理解・改造したい | [ネットワーク実装・拡張ガイド](NETWORK-DEVELOPMENT.md) |
| TCP/UDPを使う**ユーザCアプリ**を作りたい | [ソケットAPIとネットワークアプリケーション](../apps/c/NETWORK-PROGRAMMING.md) |
| カーネル全体の動作を起動から追いたい | [カーネルリーディングガイド](../kernel/KERNEL-READING-GUIDE.md) |
| TelnetやWi-Fi接続の仕様を確認したい | [ネットワークリファレンス](../../reference/network/README.md) |
| ネットワークを初めて学習したい | [教科書B6](../../learn/B/B6.md) → [教科書C5](../../learn/C/C5.md) |

**境界の原則：** `development/network/`は**カーネル側の実装方法**を担当します．`development/apps/c/NETWORK-PROGRAMMING.md`は**ユーザアプリに公開するAPIと例**を担当します．接続条件や現行制約の正本は`reference/network/`と[システムコール一覧](../../reference/SYSCALLS.md)にあります．

通常のPicoにはWi-Fiがありません．ネットワーク開発と実機実験はPico W向けの選択項目です．
