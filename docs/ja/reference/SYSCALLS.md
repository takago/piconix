# システムコール・ABIリファレンス

システムコール番号の一次資料は`include/piconix_syscall.h`の`PICONIX_SYSCALL_TABLE(X)`です．実装とAPIの説明は `kernel/kernel.c` の `os_svc_dispatch()`，`userland/lib/syscalls.c`，`userland/lib/unistd.h` に基づきます．Cアプリケーションでは通常ラッパー関数を使用します．一般的なPOSIX ABIとは異なります．

## 呼び出し規約

`svc #0` を実行し，`r0` に番号，`r1`，`r2`，`r3` に下表の引数を左から順番に格納します．戻り値は `r0` に格納されます．第4引数が必要な場合は，構造体へのポインタを渡すか，ユーザ側ラッパーで変換します．引数欄のポインタはユーザタスクのアドレス空間を指します．

通常，失敗時には負の `OS_E*` エラー番号を直接返し，`errno` は使用しません．ただし `_exit()` は復帰せず，C版 `getcwd()` は成功時に `buf`，失敗時に `NULL` を返します．予約番号11，15には実装がありません．

## 全システムコール一覧

| No. | Kernel constant | C wrapper | r1 / r2 / r3 arguments | Return | 機能 |
|---:|---|---|---|---|---|
| 1 | `SYS_GETPID` | `getpid()` | — | PID | 現在のPIDを返す |
| 2 | `SYS_KILL` | `kill(pid)` | `pid` | 0 / 負のエラー | 指定タスクを終了（自分自身は不可） |
| 3 | `SYS_EXIT` | `_exit(status)` | `status` | 復帰しない | タスクを終了する |
| 4 | `SYS_SLEEP` | `msleep(ms)` | `ms` | 0 | 指定ミリ秒スリープ |
| 5 | `SYS_YIELD` | `yield()` | — | 0 | CPUを譲る |
| 6 | `SYS_OPEN` | `open(path, flags)` | `path`, `flags` | FD / 負のエラー | ファイル・デバイスを開く |
| 7 | `SYS_CLOSE` | `close(fd)` | `fd` | 0 / 負のエラー | FDを閉じる |
| 8 | `SYS_READ` | `read(fd, buf, count)` | `fd`, `buf`, `count` | 読取バイト数 / 負のエラー | FDから読み込む |
| 9 | `SYS_WRITE` | `write(fd, buf, count)` | `fd`, `buf`, `count` | 書込バイト数 / 負のエラー | FDへ書き込む |
| 10 | `SYS_IOCTL` | `ioctl(fd, cmd, value)` | `fd`, `cmd`, `value` | 要求ごと / 負のエラー | デバイス固有の制御 |
| 11 | `SYS_RESERVED_TASK_INFO` | `—` | — | `OS_ENOSYS` | 予約番号 |
| 12 | `SYS_SPAWN` | `spawn(program, argc, argv)` | `program`, `argc`, `argv` | 子PID / 負のエラー | 登録プログラムを起動 |
| 13 | `SYS_WAITPID` | `waitpid(pid, status, options)` | `pid`, `status`, `options` | 子PID，0，または負のエラー | 子の終了を待つ |
| 14 | `SYS_SET_FG` | `set_foreground(pid)` | `pid` | 0 / 負のエラー | TTYの前景タスクを指定 |
| 15 | `SYS_RESERVED_GETARG` | `—` | — | `OS_ENOSYS` | 予約番号 |
| 16 | `SYS_CONSOLE_READY` | `console_ready()` | — | 0 または 1 | コンソール接続を確認 |
| 17 | `SYS_UNLINK` | `unlink(path)` | `path` | 0 / 負のエラー | RAMファイルを削除 |
| 18 | `SYS_DUP2` | `dup2(oldfd, newfd)` | `oldfd`, `newfd` | newfd / 負のエラー | 指定番号にFDを複製 |
| 19 | `SYS_TTY_SPAWN` | `tty_spawn(tty, program)` | `tty`, `program` | 子PID / 負のエラー | 指定TTYにタスクを起動 |
| 20 | `SYS_GET_TTY` | `get_tty()` | — | TTY識別子 | 自身のTTYを取得 |
| 21 | `SYS_LSEEK` | `lseek(fd, offset, whence)` | `fd`, `offset`, `whence` | 位置 / 負のエラー | ファイル位置を変更 |
| 22 | `SYS_CHDIR` | `chdir(path)` | `path` | 0 / 負のエラー | 作業ディレクトリを変更 |
| 23 | `SYS_GETCWD` | `getcwd(buf, size)` | `buf`, `size` | 低水準: 0 / 負のエラー | 作業ディレクトリをバッファに格納 |
| 24 | `SYS_FCNTL` | `fcntl(fd, cmd, value)` | `fd`, `cmd`, `value` | フラグまたは0 / 負のエラー | FDフラグの取得・設定 |
| 25 | `SYS_SPAWN_TMP` | `spawn_tmp(path)` | `path` | 子PID / 負のエラー | /tmpのPNX1プログラムを起動 |
| 26 | `SYS_GETDENTS` | `getdents(fd, entry, size)` | `fd`, `entry`, `size` | 1，0，または負のエラー | ディレクトリエントリを取得 |
| 27 | `SYS_FLASH_ID` | `flash_id()` | — | 識別値 | Flash識別値を取得 |
| 28 | `SYS_SOCKET` | `socket(domain, type, protocol)` | `domain`, `type`, `protocol` | FD / 負のエラー | IPv4 TCP/UDPソケットを生成 |
| 29 | `SYS_BIND` | `bind_udp(fd, port)` | `fd`, `port` | 0 / 負のエラー | UDPポートを割り当てる |
| 30 | `SYS_SENDTO` | `sendto_udp(fd, packet)` | `fd`, `packet` | 送信バイト数 / 負のエラー | UDPパケットを送る |
| 31 | `SYS_RECVFROM` | `recvfrom_udp(fd, packet)` | `fd`, `packet` | 受信バイト数 / 負のエラー | UDPパケットを受け取る |
| 32 | `SYS_LISTEN_TCP` | `tcp_listen(fd, port)` | `fd`, `port` | 0 / 負のエラー | TCP接続を待ち受ける |
| 33 | `SYS_CONNECT_TCP` | `tcp_connect(fd, endpoint)` | `fd`, `endpoint` | 0 / 負のエラー | TCP接続を開始する |
| 34 | `SYS_TCP_STATUS` | `tcp_status(fd)` | `fd` | 状態値 / 負のエラー | TCP接続状態を取得 |
| 35 | `SYS_ACCEPT_TCP` | `tcp_accept(fd)` | `fd` | 新FD / 負のエラー | 接続済みTCP FDを作成 |
| 36 | `SYS_PIPE` | `pipe(fds)` | `fds` | 0 / 負のエラー | 読み／書きFDを一組作る |
| 37 | `SYS_DUP` | `dup(oldfd)` | `oldfd` | 新FD / 負のエラー | 空き番号にFDを複製 |
| 38 | `SYS_PROGRAM_ID` | `program_id(name)` | `name` | ID / 負のエラー | 登録プログラム名をIDに変換 |
| 39 | `SYS_SHUTDOWN_TCP` | `shutdown(fd, SHUT_WR)` | `fd` | 0 / 負のエラー | TCP送信側だけを終了する |

## 引数の意味と重要な制約

- `read(fd, buf, count)` / `write(fd, buf, count)`：`fd` はファイル記述子，`buf` はデータ領域，`count` は最大バイト数です．読取時の0はEOFを表す場合があります．
- `spawn(program, argc, argv)`：`program` は登録済みプログラムID，`argc` は1〜8，`argv` は文字列ポインタ配列です．引数文字列の合計領域にも制限があります．`program_id(name)` で名前からIDを得られます．
- `waitpid(pid, status, options)`：`pid` は子PIDまたは `-1`，`status` は終了情報の格納先（`NULL`可），`options` は0または `WNOHANG` (1) です．現在の実装では完了時の `*status` に0が入るため，通常のPOSIXの終了ステータス保持とは異なります．
- `set_foreground(pid)`：`pid` は自分の子タスクのPIDです．`pid=0` は前景所有の復元要求です．
- `tty_spawn(tty, program)`：`tty` は1または2，`program` は対応するシェル／gettyの登録ID（実装では2または4）です．
- `getcwd(buf, size)`：ABIでは書き込み成功時に0を返します．Cラッパーは成功時に `buf` を返すよう変換しています．
- `lseek(fd, offset, whence)`：`/tmp`の通常ファイルと`/dev/shm`は`SEEK_SET` (0)，`SEEK_CUR` (1)，`SEEK_END` (2)に対応します．`/dev/mem`は絶対アドレスによる`SEEK_SET`のみです．
- `fcntl(fd, cmd, value)`：`cmd=1` は `F_GETFL`，`cmd=2` は `F_SETFL` に相当します．`F_SETFL`で`O_NONBLOCK`を変更できるのはTTY（コンソール，UART1，Wi-Fi有効時のWi-Fi TTY）とTCP/UDPソケットです．パイプや通常ファイル，その他のデバイスには対応していません．TTY設定には `ioctl()` を使用します．
- `pipe(fds)`：`fds[0]` に読み取り側，`fds[1]` に書き込み側のFDを設定します．`dup()`／`dup2()` はソケットFDを複製できません．
- `socket(domain, type, protocol)`：`domain=AF_INET` (2)，`type=SOCK_STREAM` (1) または `SOCK_DGRAM` (2)，`protocol=0` のみです．
- `bind_udp(fd, port)`，`tcp_listen(fd, port)`：`port` は1〜65535です．ソケットの種類によって呼び出し先が異なります．
- `sendto_udp(fd, packet)`／`recvfrom_udp(fd, packet)`：`packet` は `piconix_udp_packet_t *` で，データポインタ，長さ，相手IPアドレスとポートを保持します．送受信長には実装上の上限があります．
- `tcp_connect(fd, endpoint)`：`endpoint` は `piconix_tcp_endpoint_t *` です．接続処理は非同期であり，`tcp_status(fd)` で状態を確認します．`tcp_accept(fd)` は受け付け可能な接続がなければ `OS_EAGAIN` を返します．
- `getdents(fd, entry, size)`：`entry` は `piconix_dirent_t *` です．1はエントリ取得，0は終端を表します．
- `spawn_tmp(path)`：`path` は `/tmp` 内のPNX1形式のThumbイメージを指定します．

## C APIと低水準ABIの違い

`bind()`，`listen()`，`accept()`，`connect()`，`send()`，`recv()`，`sendto()`，`recvfrom()` はユーザ側のBSD風アダプターで，個々に独立したSVC番号はありません．特にTCPの `bind()` は待受開始を行い，`listen()` はバックログ管理を実装しません．`accept()` は相手アドレスの取得に対応していません．詳しくは [NETWORK-PROGRAMMING.md](../development/apps/c/NETWORK-PROGRAMMING.md) を参照してください．

## 参照先

- [LIBC.md](../development/apps/c/LIBC.md)：ユーザ向けCライブラリと関数群
- [ASM.md](../development/apps/asm/ASM.md)：アセンブリからのSVC呼び出し
- [PROGRAMMING.md](../development/apps/c/PROGRAMMING.md)：アプリ開発の手順
- [TTY.md](../literacy/TTY.md)：TTY制御

`getdents()` の結果に`device_id`を追加しました．デバイスIDがない場合は`0xffff`です．

`getdents()`の`device_id`フィールドはディレクトリ項目のIDです．`/dev`と`/proc`ではデバイスID，`/bin`ではプログラムID，`/tmp`・`/etc`ではRAMファイルのスロット番号（0始まり）を返します．ディレクトリやIDのない項目は`0xffff`です．`ls -l`は`TYPE ID SIZE NAME`の見出しを表示し，`p`をプログラム，`f`をRAMファイル，`c`をデバイス，`d`をディレクトリとして区別します．スロット番号はファイルの削除・再生成で変わるため，恒久的な識別子ではありません．
