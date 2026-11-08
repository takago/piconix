# PicoNIX 開発履歴 / Development History

このディレクトリには過去の設計案，実装段階，障害調査の記録を保存しています．**現行仕様ではありません**．現在の使い方と仕様は[日本語の文書案内](../DOCS.ja.md)，[English documentation](../DOCS.md)，[日本語仕様書](../ja/reference/SPECIFICATION.md)，[English specification](../en/reference/SPECIFICATION.md)を参照してください．

This directory preserves earlier designs, implementation stages, and debugging notes. **These documents are historical, not current specifications.** Follow the current guides and source code for actual behavior.

## 日本語の開発記録

- [プログラム登録数と端末表示の改善](ja/CHANGE-NOTES.md) — `CHANGE-NOTES.md`
- [Pico W DHCP実装・障害調査メモ](ja/NETWORK-DHCP-NOTES.md) — `NETWORK-DHCP-NOTES.md`
- [シンボル情報の階層化](ja/PROC-SYM.md) — `PROC-SYM.md`

## English development notes

- [Program Capacity and Terminal Display Improvements](en/CHANGE-NOTES.md) — `CHANGE-NOTES.md`
- [FD table and descriptor allocation](en/FD16-DUP.md) — `FD16-DUP.md`
- [Experimental nc command (stage 9)](en/NC_EXPERIMENTAL.md) — `NC_EXPERIMENTAL.md`
- [Experimental socket API (stage 8)](en/NETWORK_SOCKET_EXPERIMENTAL.md) — `NETWORK_SOCKET_EXPERIMENTAL.md`
- [Stage 19: Remote device service refactoring](en/NETWORK_STAGE19.md) — `NETWORK_STAGE19.md`
- [Stage 21: send/recv and echo daemons](en/NETWORK_STAGE21.md) — `NETWORK_STAGE21.md`
- [Experimental TCP client backend (stage 6)](en/NETWORK_STAGE6.md) — `NETWORK_STAGE6.md`
- [Pico W RAM layout (stage 1 only)](en/PICO-W-PLAN.md) — `PICO-W-PLAN.md`
- [Pipe integration: implementation gate](en/PIPE-DESIGN-STAGE2.md) — `PIPE-DESIGN-STAGE2.md`
- [Pipe implementation audit (work in progress)](en/PIPE-IMPLEMENTATION-NOTES.md) — `PIPE-IMPLEMENTATION-NOTES.md`
- [Pipe readability refactor](en/PIPE-REFACTOR.md) — `PIPE-REFACTOR.md`
- [Pipe stage 3: kernel foundation (not yet a working shell pipeline)](en/PIPE-STAGE3.md) — `PIPE-STAGE3.md`
- [Pipe stage 4: scheduler integration](en/PIPE-STAGE4.md) — `PIPE-STAGE4.md`
- [Experimental UDP socket syscall bridge (stage 3)](en/SOCKET-EXPERIMENTAL.md) — `SOCKET-EXPERIMENTAL.md`
- [Minimal BSD-style socket compatibility API](en/SOCKET_COMPAT.md) — `SOCKET_COMPAT.md`
- [Experimental TCP accept (stage 7)](en/TCP_ACCEPT_STAGE7.md) — `TCP_ACCEPT_STAGE7.md`
- [TCP backend stage 4 (experimental)](en/TCP_BACKEND_STAGE4.md) — `TCP_BACKEND_STAGE4.md`
- [Stage 5: experimental TCP FD integration](en/TCP_FD_STAGE5.md) — `TCP_FD_STAGE5.md`
- [Three-stage pipelines](en/THREE-STAGE-PIPE.md) — `THREE-STAGE-PIPE.md`
- [Socket descriptors in lsof and trace (experimental)](en/TRACE_SOCKET_FDS.md) — `TRACE_SOCKET_FDS.md`

## 端末の調査記録 / Terminal debugging

- [日本語：端末表示の調査記録](ja/TERMINAL-DEBUG.md)
- [English: Terminal display debugging](en/TERMINAL-DEBUG.md)

## 旧学習コースについて / Retired learning paths

旧学習コース案内と重複した基礎実習は，現行の[日本語ロードマップ](../ja/ROADMAP.md)・[English learning roadmap](../en/ROADMAP.md)へ統合しました．古いファイルを残して混乱を招かないよう，過去版はGitのコミット履歴で参照してください．

Previous learning-path files and duplicated foundational exercises have been integrated into the current learning roadmap. Consult Git history if the older versions are needed.
