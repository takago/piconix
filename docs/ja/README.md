# PicoNIX ドキュメント案内

PicoNIXの文書は，**セットアップ，技術リファレンス，開発者向け資料**に分かれています．各文書は現行実装の仕様・制限・操作例を説明します．変更履歴と旧仕様は[開発日誌](DEVELOPMENT_DIARY.md)にまとめています．

**利用条件：** カーネル・アプリ開発資料は，C言語とUNIX/Linuxの基本操作，プロセス・FD・システムコールの知識を前提にしています．PicoNIXとLinuxの差異は[互換性の説明](reference/DIFFERENCES.md)を参照してください．

## セットアップ

[セットアップと書き込み](setup/SETUP.md)：開発環境，ビルド，Pico／Pico Wへの書き込み，端末接続．

## 技術リファレンス

[リファレンス総目次](reference/README.md)：API，システムコール，FD，メモリ配置，デバイス，ネットワーク，実装上の制限．

分野別：[コマンド](reference/commands/README.md) ｜ [ハードウェアと配線](reference/hardware/README.md) ｜ [デバイスとI/O](reference/devices/README.md) ｜ [ネットワーク（Pico W）](reference/network/README.md)．

## 開発者向け資料

[開発資料一覧](development/README.md)：C・Python・アセンブラのAPIと実装例，カーネル構造，システムコール・デバイス・ネットワークの拡張手順．

## 目的別索引

[目的別索引](TASKS.md)：具体的な操作・仕様・ソースの機能名から参照先を探せます．

## 関数索引と開発記録

- [関数一覧（自動生成）](development/FUNCTIONS.md)：ソースコードを調べるときの索引．
- [開発日誌](DEVELOPMENT_DIARY.md)：設計の経緯と仕様変更の履歴．

---

[プロジェクト概要](../../README.md) ｜ [English](../en/README.md) ｜ [開発日誌](DEVELOPMENT_DIARY.md)
