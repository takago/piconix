# コマンドリファレンス

この一覧は，`CMakeLists.txt`で標準組込み対象として登録されている**全53個のアプリケーション**と，シェル組込みコマンドを対象とします．一般的なUNIXのコマンドと同名でも，PicoNIXの引数や機能は限定されています．

構文は要約です．`help NAME`でファームウェア内の簡易ヘルプを確認できます．詳しい操作を必要とするコマンドは「詳細」から専用文書を参照してください．ネットワーク機能の一部はPico W専用です．

**実装の基準：** [`CMakeLists.txt`](../../../../CMakeLists.txt) の `foreach(APP IN ITEMS ...)`， [`help.c`](../../../../userland/apps/help.c)，および [`shell.c`](../../../../userland/apps/shell.c)．

## ファイル・シェル

| コマンド | 用途 | 構文・主要オプション | 詳細 |
|---|---|---|---|
| [`sh`](../../../../userland/apps/shell.c) | 対話シェル（表示名 sh） | `sh` | [解説](PIPE.md) |
| [`ls`](../../../../userland/apps/ls.c) | ファイル一覧 | `ls [-la] [path]（-l: 詳細，-a: .と..）` | — |
| [`tree`](../../../../userland/apps/tree.c) | ディレクトリをツリー表示 | `tree [-L 0..8] [path]` | — |
| [`find`](../../../../userland/apps/find.c) | ファイル名を検索 | `find [path] [-name pattern]` | — |
| [`fd`](../../../../userland/apps/fd.c) | ファイルブラウザ | `fd [directory]` | [解説](FD.md) |
| [`pwd`](../../../../userland/apps/pwd.c) | 現在のディレクトリを表示 | `pwd` | — |
| [`cat`](../../../../userland/apps/cat.c) | ファイル・標準入力の表示 | `cat [-n] [file]（-n: 行番号）` | — |
| [`cp`](../../../../userland/apps/cp.c) | RAMファイルのコピー | `cp SRC DST` | — |
| [`rm`](../../../../userland/apps/rm.c) | RAMファイルを削除 | `rm path` | — |
| [`echo`](../../../../userland/apps/echo.c) | 文字列の出力 | `echo [-n] [-e] [text...]（-n: 改行なし，-e: エスケープ）` | — |
| [`grep`](../../../../userland/apps/grep.c) | 部分文字列を含む行を抽出 | `grep PATTERN [file]` | — |
| [`hexdump`](../../../../userland/apps/hexdump.c) | 16進数・ASCII表示 | `hexdump [file [offset [count]]]` | — |
| [`dd`](../../../../userland/apps/dd.c) | バイト列・RAMデータ転送 | `dd [if=FILE] [of=FILE] [bs=1..256] [count=N] [skip=N] [seek=N]` | [解説](DD.md) |
| [`edit`](../../../../userland/apps/edit.c) | 行編集型テキストエディタ | `edit /tmp/FILE \| edit /etc/FILE` | [解説](EDIT.md) |
| [`vi`](../../../../userland/apps/vi.c) | モード型テキストエディタ | `vi file` | [解説](VI.md) |
| [`help`](../../../../userland/apps/help.c) | コマンド一覧と簡易ヘルプ | `help [command]` | [解説](HELP.md) |

## システム・端末

| コマンド | 用途 | 構文・主要オプション | 詳細 |
|---|---|---|---|
| [`ps`](../../../../userland/apps/ps.c) | プロセス一覧 | `ps` | — |
| [`kill`](../../../../userland/apps/kill.c) | プロセスを停止 | `kill PID` | — |
| [`pty_test`](../../../../userland/apps/pty_test.c) | PTY経由でコマンドを対話操作 | `pty_test [command [args...]]` | [PTY](../devices/PTY.md) |
| [`tty`](../../../../userland/apps/tty.c) | 制御端末を表示 | `tty` | [解説](TTY.md) |
| [`stty`](../../../../userland/apps/stty.c) | 端末モードを設定 | `stty [raw\|cooked\|icanon\|-icanon\|echo\|-echo\|onlcr\|-onlcr] ...` | [解説](TTY.md) |
| [`reset`](../../../../userland/apps/reset.c) | 端末状態の復旧 | `reset` | [解説](TERMINAL.md) |
| [`lsof`](../../../../userland/apps/lsof.c) | 開いているFDを表示 | `lsof [PID]` | — |
| [`trace`](../../../../userland/apps/trace.c) | システムコールとカーネルイベントの追跡 | `trace on [-s\|-a] \| off \| show [PID] \| follow [PID] \| run [-s\|-a] CMD ...` | [解説](TRACE.md) |
| [`watch`](../../../../userland/apps/watch.c) | コマンドの繰り返し実行 | `watch [seconds] command [args...]` | — |
| [`mem`](../../../../userland/apps/mem.c) | メモリビューア | `mem [address] \| mem -s [app]` | [解説](MEM.md) |

## ハードウェア

| コマンド | 用途 | 構文・主要オプション | 詳細 |
|---|---|---|---|
| [`pins`](../../../../userland/apps/pins.c) | ピン配置の表示 | `pins` | [解説](../hardware/PINS.md) |
| [`gpio`](../../../../userland/apps/gpio.c) | 対話型GPIO操作 | `gpio` | [解説](../devices/README.md) |
| [`i2cctl`](../../../../userland/apps/i2cctl.c) | I²C1デバイス操作 | `i2cctl scan \| read ADDR COUNT \| write ADDR BYTE... \| xfer ADDR READ_COUNT [BYTE...]` | [解説](../devices/I2C.md) |
| [`bh1750`](../../../../userland/apps/bh1750.c) | BH1750照度センサの読み出し | `bh1750 [count]（0: 連続）` | [解説](../devices/I2C.md) |
| [`fbctl`](../../../../userland/apps/fbctl.c) | 8×8フレームバッファ制御 | `fbctl on\|off\|clear\|status\|test` | [解説](../devices/README.md) |
| [`fbed`](../../../../userland/apps/fbed.c) | 対話型フレームバッファ編集 | `fbed` | [解説](../devices/README.md) |
| [`pwmctl`](../../../../userland/apps/pwmctl.c) | PWM制御 | `pwmctl status\|freq HZ\|a DUTY\|b DUTY\|off` | [解説](../devices/README.md) |
| [`blink`](../../../../userland/apps/blink.c) | LED点滅タスク | `blink [&]` | — |

## プログラミング・ネットワーク

| コマンド | 用途 | 構文・主要オプション | 詳細 |
|---|---|---|---|
| [`as`](../../../../userland/apps/asm.c) | Thumbアセンブラ（表示名 as） | `as file.s` | [解説](../../development/apps/asm/ASM.md) |
| [`cc`](../../../../userland/apps/cc.c) | Cサブセットコンパイラ | `cc source.c` | [解説](../../development/apps/c/CC.md) |
| [`python`](../../../../userland/apps/python.c) | 内蔵Pythonインタプリタ | `python [file]` | [解説](../../development/apps/python/PYTHON.md) |
| [`net`](../../../../userland/apps/net.c) | Wi-Fi状態・接続端末・ARP表 | `net [-a\|-r]` | [解説](../network/NET.md) |
| [`ip`](../../../../userland/apps/ip.c) | IPインタフェースの表示 | `ip [addr]` | [解説](../network/NETWORK.md) |
| [`nc`](../../../../userland/apps/nc.c) | TCP/UDP通信 | `nc [-u] [-N] -l PORT \| nc [-u] [-N] IPv4 PORT` | [解説](../network/NC.md) |
| [`ping`](../../../../userland/apps/ping.c) | ICMP応答確認 | `ping [-c COUNT] IPv4` | [解説](../network/PING.md) |
| [`tcpdump`](../../../../userland/apps/tcpdump.c) | IPv4パケットを観察 | `tcpdump [-x] [-c COUNT]` | [解説](../network/TCPDUMP.md) |
| [`httpd`](../../../../userland/apps/httpd.c) | HTTP/1.0サーバ（TCP 80） | `httpd` | [解説](../network/HTTPD.md) |
| [`tcp_echod`](../../../../userland/apps/tcp_echod.c) | TCPエコーサーバ | `tcp_echod [PORT]（既定7）` | [解説](../network/NETWORK.md) |
| [`udp_echod`](../../../../userland/apps/udp_echod.c) | UDPエコーサーバ | `udp_echod [PORT]（既定7）` | [解説](../network/NETWORK.md) |
| [`mcpd`](../../../../userland/apps/mcpd.c) | MCP HTTPサーバ（TCP 8080） | `mcpd` | [解説](MCP.md) |
| [`scroll`](../../../../userland/apps/scroll.c) | LED文字スクロール（標準入力・UDP） | `scroll [-u [PORT]]（既定UDP 2001）` | [解説](SCROLL.md) |

## デモ・ゲーム

| コマンド | 用途 | 構文・主要オプション | 詳細 |
|---|---|---|---|
| [`hello`](../../../../userland/apps/hello.c) | 挨拶表示 | `hello [name]` | — |
| [`mb`](../../../../userland/apps/mb.c) | Moon Buggyゲーム | `mb` | — |
| [`pong`](../../../../userland/apps/pong.c) | Pongゲーム | `pong [-2]` | [解説](PONG.md) |
| [`reversi`](../../../../userland/apps/reversi.c) | コンピュータ対戦のリバーシ | `reversi` | [解説](REVERSI.md) |
| [`omikuji`](../../../../userland/apps/omikuji.c) | おみくじ・入力デモ | `omikuji [-i \| seconds]` | — |
| [`kitesys`](../../../../userland/apps/kitesys.c) | 組込みシステムのデモ | `kitesys` | — |

## シェル組込みコマンド

| コマンド | 役割 | 構文 |
|---|---|---|
| `cd` | 作業ディレクトリを変更 | `cd [path]` |
| `getty` | 端末にシェルまたはPythonを接続 | `getty /dev/ttyS1 sh\|python [&]` |
| `exit` | シェルを終了 | `exit` |
| `logout` | シェルを終了 | `logout` |

これらは独立したPICアプリではなく，[`shell.c`](../../../../userland/apps/shell.c)内で処理されます．パイプ・リダイレクト・バックグラウンド起動の制約は[PIPE.md](PIPE.md)を参照してください．

**補足：** `help`にはコマンドの簡易説明がありますが，この表の「標準搭載」判定はビルド設定を基準にしています．個別コマンドのオプションは実装変更で変わるため，疑義がある場合は表内のソースリンクを確認してください．

[技術リファレンス](../README.md) ｜ [English](../../../en/reference/commands/README.md)
