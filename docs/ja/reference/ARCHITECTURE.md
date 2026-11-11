# PicoNIX 全体アーキテクチャ

*技術リファレンス*

この文書は，現行ソースコードを読み始めるための**構成図と参照先**を示します．内部ABIやメモリ上限は[仕様書](SPECIFICATION.md)，各機能の詳細は個別ガイドを参照してください．

## 1．実行構造

```text
                ユーザ空間（24 KiB × 6スロット）
             sh | grep | mem | python | httpd | ...
                       | syscall (SVC)
                       v
+---------------------------------------------------------+
| カーネル                                                 |
| タスク / SysTick / PendSV / FD / パイプ / VFS / TTY      |
| ソケット管理 / Telnet / UDPデバイス制御                  |
+---------------------------------------------------------+
         |                |                |
     USB / UART        GPIO等         lwIP / Wi-Fi (Pico W)
         |                |                |
                  RP2040・Pico SDK
```

登録済みのユーザアプリはFlashにPICイメージとして保持し，実行時にRAMスロットへコピーします．カーネルコードはXIP実行です．MMU・仮想記憶・プロセスごとのメモリ保護はありません．

## 2．主なソースファイル

| ソース | 責務 |
|---|---|
| `kernel/main.c` | 起動・組込みアプリの登録 |
| `kernel/kernel.c` | タスク，FD，パイプ，VFS，TTY，SVCの主処理 |
| `kernel/context.S` | コンテキスト切替 |
| `kernel/gpio.c`，`fb.c`，`adc.c`，`pwm.c` | デバイス実装 |
| `kernel/net.c`，`net_socket.c` | ネットワークとソケットの基盤 |
| `kernel/net_service_telnet.c`，`net_service_remote_devctl.c` | カーネル側ネットワークサービス |
| `userland/apps/shell.c` | コマンド解釈，リダイレクト，パイプライン，バックグラウンド |
| `userland/lib/syscalls.c`，`crt0.c` | ユーザAPIと起動処理 |
| `userland/apps/httpd.c`，`nc.c` | ユーザ空間ネットワークアプリ |
| `include/piconix.h` | 公開APIとシステムコール関連定義 |
| `CMakeLists.txt`，`tools/build_sdk.sh` | ビルド，組込みイメージ，リンク後検証 |

## 3．ファイル・端末・通信の境界

- `/bin`はFlashに埋め込んだアプリ一覧，`/tmp`と`/etc`は簡易RAMファイル領域，`/dev`はデバイス，`/proc`は状態・シンボル情報を提供します．詳細は[ファイルシステム](FILESYSTEM.md)を参照してください．
- 標準FDは通常0，1，2です．デバイス・ファイル・パイプ・ソケットをFDとして扱いますが，POSIXとの完全互換性はありません．詳細は[パイプ](commands/PIPE.md)と[システムコール](SYSCALLS.md)を参照してください．
- TTYの入力設定・ONLCR等はカーネル，画面のカーソル・代替画面等はアプリとホスト端末が管理します．[TTY](commands/TTY.md)と[画面制御](commands/TERMINAL.md)を参照してください．
- Pico WのTelnet（TCP 23）とUDPデバイス制御（UDP 2000）はカーネル側，HTTP（TCP 80）はユーザ空間の`httpd`です．Wi-Fi動作はビルド構成に依存します．[ネットワーク](network/NETWORK.md)を参照してください．

## 4．実装ファイルの参照先

初めて読む場合は，`kernel/main.c` → `userland/apps/shell.c` → `userland/lib/syscalls.c` → `kernel/kernel.c` → `kernel/context.S`の順が理解しやすいでしょう．ネットワークは`kernel/net_socket.c`，デバイスは各ドライバへ進みます．変更時には日英双方の文書と`userland/apps/help.c`の整合性も確認してください．

---

**関連リンク：** [リファレンス目次](README.md) ｜ [English](../../en/reference/ARCHITECTURE.md) ｜ [内部構造](INTERNAL.md) ｜ [文書案内](../README.md)
