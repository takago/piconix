# ソースコードを起点にした読解実習

UNIX/Linuxの基本操作とC言語を既習の学生向けです．**実行 → システムコール → カーネル内の状態変化 → 戻り値**を同じ題材で追います．出力のPIDや時刻は機器ごとに変わります．

| 課題 | PicoNIXで観察 | 読むソース | 確認すること |
|---|---|---|---|
| 起動からシェル | `ps`，`ls /bin` | `kernel/main.c`，`kernel/kernel.c`，`userland/apps/shell.c` | `register_image()`とタスク生成，`/bin`の対応 |
| FDとVFS | `echo hello > /tmp/a`，`cat /tmp/a` | `userland/lib/syscalls.c`，`include/piconix_syscall.h`，`kernel/context.S`，`kernel/kernel.c` | `open/write/read/close`のSVC番号，FDとファイルの関係 |
| パイプ | `echo hello | cat -n` | `userland/apps/shell.c`，`kernel/kernel.c` | `pipe/dup2/close`とEOFの成立条件 |
| 入力待ち | `trace run -s hello` | `userland/apps/trace.c`，`kernel/kernel.c`，`include/piconix_trace.h` | ENTERとEXITを区別する．`select()`は40番 |
| デバイス | `cat /dev/fbchar`，`cat /proc/fb` | `include/device_names.h`，`kernel/kernel.c`，`kernel/fb.c` | VFSでパスがデバイス処理へ到達する経路．実際のLED駆動は配線確認後 |
| RAMとPIC | `cat /proc/memory`，`mem` | `userland/lib/slot.ld`，`kernel/main.c`，`kernel/kernel.c`，`tools/test_ram_pipeline.py` | XIPと24 KiBスロット，4 KiBスタック予約，PIC検査 |
| ネットワーク（Pico W） | `ip`，`ping 127.0.0.1` | `kernel/net.c`，`kernel/net_socket.c`，`userland/apps/ping.c` | カーネル内lwIPとユーザソケットの境界 |
| MCP（Pico W） | `mcpd &`，Linux側の`host/mcp_client.py` | `userland/apps/mcpd.c`，`host/mcp_client.py` | TCP・HTTP・JSON-RPCとMCPの役割分担 |

## 読解手順

1. まず[操作ガイド](../guides/README.md)でコマンドの引数と注意事項を確認する．
2. [システムコール一覧](../reference/SYSCALLS.md)で番号・APIを調べ，`include/piconix_syscall.h`と突き合わせる．
3. [関数索引](../../FUNCTIONS.md)から関数を探す．行番号はソース変更後に再生成する．
4. 戻り値・エラー・ブロッキング条件をコードで追い，[Linuxとの差異](../reference/DIFFERENCES.md)と比較する．
5. コードを変更する演習は別ブランチで行い，`python3 tools/run_tests.py`，必要に応じてSDKビルド・実機テストを実施する．

[演習一覧](README.md) ｜ [カーネル読解ガイド](../development/kernel/KERNEL-READING-GUIDE.md) ｜ [English](../../en/exercises/READING-PATHS.md)
