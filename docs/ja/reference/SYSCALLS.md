# システムコール・ABIリファレンス

*技術リファレンス*

システムコール番号の一次資料は`include/piconix_syscall.h`の`PICONIX_SYSCALL_TABLE(X)`です．実装とAPIの説明は `kernel/kernel.c` の `os_svc_dispatch()`，`userland/lib/syscalls.c`，`userland/lib/unistd.h` に基づきます．Cアプリケーションでは通常ラッパー関数を使用します．一般的なPOSIX ABIとは異なります．

## 呼び出し規約

`svc #0` を実行し，`r0` に番号，`r1`，`r2`，`r3` に下表の引数を左から順番に格納します．戻り値は `r0` に格納されます．第4引数が必要な場合は，構造体へのポインタを渡すか，ユーザ側ラッパーで変換します．引数欄のポインタはユーザタスクのアドレス空間を指します．

通常，失敗時には負の `OS_E*` エラー番号を直接返し，`errno` は使用しません．ただし `_exit()` は復帰せず，C版 `getcwd()` は成功時に `buf`，失敗時に `NULL` を返します．予約番号11，15には実装がありません．

## 機能別クイック索引

次の表は番号表を目的別に引き直したものです．**番号表には予約番号も含む40エントリがあり，実際に呼び出せるのは38個**です．関数名や引数，戻り値は直後の全一覧で確認してください．

| 分野 | 番号 | 主なAPI・用途 | 関連資料 |
|---|---|---|---|
| タスクと時間 | 1–5，12–14，19–20，25，38 | `getpid`，`kill`，`_exit`，`msleep`，`yield`，`spawn`，`waitpid`，前景TTY，プログラムID | [アーキテクチャ](ARCHITECTURE.md)，[TTY](commands/TTY.md) |
| ファイル・FD・ディレクトリ | 6–9，17–18，21–24，26，36–37 | `open`，`read`，`write`，`lseek`，`dup2`，`pipe`，`getdents`など | [ファイルシステム](FILESYSTEM.md)，[パイプ](commands/PIPE.md) |
| デバイス・端末 | 10，16，27 | `ioctl`，コンソール接続確認，Flash ID | [デバイス一覧](devices/README.md)，[TTY](commands/TTY.md) |
| ネットワーク・多重化 | 28–35，39–40 | `socket`，UDP，TCP，`select` | [ネットワーク](network/NETWORK.md)，[ソケット開発](../development/apps/c/NETWORK-PROGRAMMING.md) |
| 予約 | 11，15 | 使用不可（`OS_ENOSYS`） | [ABI定義](../../../include/piconix_syscall.h) |

**注意：** 番号順と分類は一致しません．Cのラッパー関数とSVC番号も一対一とは限りません．たとえば`send()`と`recv()`は独立したSVCではありません．

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
| 40 | `SYS_SELECT` | `select(nfds, readfds, writefds, NULL, timeout)` | `nfds`, `piconix_select_request_t *`, — | 準備済FD数 / 負のエラー | 最大16FDの読み書き準備を待機 |

## 引数の意味と重要な制約

- `read(fd, buf, count)` / `write(fd, buf, count)`：`fd` はファイル記述子，`buf` はデータ領域，`count` は最大バイト数です．読取時の0はEOFを表す場合があります． TTY（PTY Slaveを含む）では非ブロッキング入力が未準備のときも`read()`は0を返します（canonical・非canonical共通）．TTYへの`write()`は部分書込みになる場合があり，USB CDCの送信キュー満杯時は0，Wi-Fi TTYでは`OS_EAGAIN`や`OS_EIO`を返す場合があります．詳細は[TTYガイド](commands/TTY.md)を参照してください．
- `spawn(program, argc, argv)`：`program` は登録済みプログラムID，`argc` は1〜8，`argv` は文字列ポインタ配列です．引数文字列の合計領域にも制限があります．`program_id(name)` で名前からIDを得られます．
- `waitpid(pid, status, options)`：`pid` は子PIDまたは `-1`，`status` は終了情報の格納先（`NULL`可），`options` は0または `WNOHANG` (1) です．完了時の `*status` には子の終了コード（0〜255）を直接格納します．POSIXの`WEXITSTATUS`等を使う符号化済みステータスではありません．
- `set_foreground(pid)`：`pid` は自分の子タスクのPIDです．`pid=0` は前景所有の復元要求です．
- `tty_spawn(tty, program)`：`tty` は1または2，`program` は対応するシェル／gettyの登録ID（実装では2または4）です．
- `getcwd(buf, size)`：ABIでは書き込み成功時に0を返します．Cラッパーは成功時に `buf` を返すよう変換しています．
- `lseek(fd, offset, whence)`：`/tmp`の通常ファイル，`/dev/shm`，読み取り専用の`/proc/sym/kernel`は`SEEK_SET` (0)，`SEEK_CUR` (1)，`SEEK_END` (2)に対応します．シンボル表は公開テキスト内のバイト位置で指定し，EOFを超える移動は拒否します．`/dev/mem`は絶対アドレスによる`SEEK_SET`のみです．`/proc/sym/userland/*`を含むその他の`/proc`ファイルはシーク非対応です．
- `fcntl(fd, cmd, value)`：`cmd=1` は `F_GETFL`，`cmd=2` は `F_SETFL` に相当します．`F_SETFL`で`O_NONBLOCK`を変更できるのはTTY（コンソール，UART1，Wi-Fi有効時のWi-Fi TTY），PTY，パイプ，TCP/UDPソケットです．通常ファイルなどには対応していません．TTY設定には `ioctl()` を使用します．
- `pipe(fds)`：`fds[0]` に読み取り側，`fds[1]` に書き込み側のFDを設定します．`dup()`／`dup2()` はソケットFDを複製できません．
- `socket(domain, type, protocol)`：`domain=AF_INET` (2)，`type=SOCK_STREAM` (1) または `SOCK_DGRAM` (2)，`protocol=0` のみです．
- `bind_udp(fd, port)`，`tcp_listen(fd, port)`：`port` は1〜65535です．ソケットの種類によって呼び出し先が異なります．
- `sendto_udp(fd, packet)`／`recvfrom_udp(fd, packet)`：`packet` は `piconix_udp_packet_t *` で，データポインタ，長さ，相手IPアドレスとポートを保持します．送受信長には実装上の上限があります．
- `tcp_connect(fd, endpoint)`：`endpoint` は `piconix_tcp_endpoint_t *` です．接続処理は非同期であり，`tcp_status(fd)` で状態を確認します．`tcp_accept(fd)` は既定では接続までタスクをブロックし，`O_NONBLOCK` 設定時に接続がなければ `OS_EAGAIN` を返します．
- `getdents(fd, entry, size)`：`entry` は `piconix_dirent_t *` です．1はエントリ取得，0は終端を表します．
- `spawn_tmp(path)`：`path` は `/tmp` 内のPNX1形式のThumbイメージを指定します．

- TCPの`read()`とUDPの`recvfrom()`，TCPの`accept()`は既定でブロッキング動作し，未準備ならタスクを`BLOCKED`にします．`fcntl(F_SETFL)`で`O_NONBLOCK`を設定すると，未準備の場合は`OS_EAGAIN`を返します．TCPの`write()`／送信と`connect()`は同じ待機規則ではありません．UDPの短すぎる受信バッファは`OS_EINVAL`で，データグラムは保持されます．TCPの正常終端を読み切った後は`read()`が0を返します．

## `open()`，`unlink()`，`getdents()`の境界条件

- `open()`で指定するフラグは`O_RDONLY`，`O_WRONLY`，`O_RDWR`と，対応する`O_CREAT`，`O_TRUNC`，`O_APPEND`，`O_NONBLOCK`の組合せに限定されます．不正な組合せは`OS_EINVAL`を返します．`O_NONBLOCK`を`open()`時に指定できるのはコンソール，UART1，PTYのMaster/Slaveです（Wi-Fi端末を含め，対応するTTYやソケットでは開いた後に`fcntl(F_SETFL)`で変更できます）．
- `/tmp`と`/etc`のファイルが存在せず，`O_CREAT`も指定しなければ`OS_ENOENT`です．11個のRAMファイル枠がすべて使用中なら，新規作成は`OS_ENOSPC`を返します．利用可能なFDがなければ`OS_EMFILE`です．RAMファイル名はASCIIで最大27バイト（内部形式の`tmp/`または`etc/`を含めて31バイト）です．
- `/tmp`と`/etc`で`O_TRUNC`は書込可能モードで開いた場合だけサイズを0にします．`O_APPEND`では各書込み時にファイル末尾を使用します．2 KiBの上限では部分書込みが可能ですが，書き込めるバイトが0のときは`OS_ENOSPC`を返します．
- `unlink()`でRAMファイルを削除するとき，どれかのタスクがそのファイルを開いていれば`OS_EBUSY`です．存在しなければ`OS_ENOENT`です．
- `getdents()`はディレクトリFDにのみ有効で，正常時は1件につき`1`，末尾で`0`を返します．サイズが`sizeof(piconix_dirent_t)`未満の場合は`OS_EINVAL`，ディレクトリではないFDの場合は`OS_EBADF`です．
- `lseek()`は対応するファイルに限定されます．RAMファイルと`/dev/shm`はサイズ末尾を超えた位置までシークできますが，それぞれ2 KiBを超える位置は`OS_EINVAL`です．シーク非対応のFDも`OS_EINVAL`を返します．

## プロセス・FD操作のエラー条件と資源管理

- `spawn(program, argc, argv)`は未登録IDに`OS_ENOENT`，不正な`argc`（1～8以外）・ポインタ・引数領域（文字列合計128バイト超など）に`OS_EINVAL`，6個のユーザスロットが埋まっている場合に`OS_EAGAIN`を返します．子には親の標準FD 0～2だけが継承され，それ以外のFDは継承されません．
- `waitpid(pid, status, options)`は自分の子のPIDまたは`-1`を受け付けます．`options`は0か`WNOHANG`だけです．不正引数は`OS_EINVAL`，対象の実行中の子も未回収の終了通知もなければ`OS_ESRCH`です．`WNOHANG`で子が実行中なら0を返します．終了済みの子はタスクスロットを占有せず，通知からPIDを1回だけ取得できます．終了通知は最大6件で，満杯の場合は配列の先頭の通知を上書きするため，未回収の終了情報が失われる可能性があります．`status`には`_exit(code)`から渡された下位8ビット（0〜255）を格納します．`kill()`による強制終了は現行実装で143（128+15）です．
- `kill(pid)`は指定PIDを終了させ，成功時は0を返します．該当PIDなし，自分自身，アイドルタスクへの指定は`OS_ESRCH`です．一般的なPOSIXシグナル配信ではありません．
- `pipe(fds)`は成功時0です．無効な格納先は`OS_EINVAL`，FDの空きが2つ未満なら`OS_EMFILE`，2本のパイプが使用中なら`OS_EAGAIN`を返します．パイプが空のときの読取り，満杯時の書込みは待機し，すべての書込み端が閉じられた空のパイプはEOF（0），読み手のないパイプへの書込みは`OS_EIO`です．
- `dup(oldfd)`は空きFDを割り当て，`dup2(oldfd, newfd)`は指定FDを置き換えます．不正なFDは`OS_EBADF`，`dup()`で空きがなければ`OS_EMFILE`です．ソケットを複製元または`dup2()`の置換先とする操作は`OS_EINVAL`です．同じFDへの`dup2(fd, fd)`は有効なFDならその番号を返します．パイプ端点の参照数は複製，継承，`close()`，タスク終了時に調整されます．

## PTY（擬似端末）

`open("/dev/ptmx", O_RDWR)`で1組のMasterを確保し，`/dev/pts/0`をSlaveとして開きます．`dup2()`で子のFD 0〜2をSlaveへ付け替えてから`spawn_foreground()`すると，子の制御TTYはTTY 3になります．Masterの`read()`はSlaveの出力，`write()`はSlaveへの端末入力です．`select()`，`fcntl(O_NONBLOCK)`，SlaveのTTY用`ioctl()`に対応しています．詳細は[PTYリファレンス](devices/PTY.md)を参照してください．なお，PTY操作専用のシステムコールは追加していません．

## C APIと低水準ABIの違い

`bind()`，`listen()`，`accept()`，`connect()`，`send()`，`recv()`，`sendto()`，`recvfrom()` はユーザ側のBSD風アダプターで，個々に独立したSVC番号はありません．特にTCPの `bind()` は待受開始を行い，`listen()` はバックログ管理を実装しません．`accept()` は相手アドレスの取得に対応していません．詳しくは [NETWORK-PROGRAMMING.md](../development/apps/c/NETWORK-PROGRAMMING.md) を参照してください．

## 参照先

- [LIBC.md](../development/apps/c/LIBC.md)：ユーザ向けCライブラリと関数群
- [ASM.md](../development/apps/asm/ASM.md)：アセンブリからのSVC呼び出し
- [PROGRAMMING.md](../development/apps/c/PROGRAMMING.md)：アプリ開発の手順
- [TTY.md](commands/TTY.md)：TTY制御


`getdents()`の`device_id`フィールドはディレクトリ項目のIDです．`/dev`と`/proc`ではデバイスID，`/bin`ではプログラムID，`/tmp`・`/etc`ではRAMファイルのスロット番号（0始まり）を返します．ディレクトリやIDのない項目は`0xffff`です．`ls -l`は`TYPE ID SIZE NAME`の見出しを表示し，`p`をプログラム，`f`をRAMファイル，`c`をデバイス，`d`をディレクトリとして区別します．スロット番号はファイルの削除・再生成で変わるため，恒久的な識別子ではありません．


`spawn_foreground()`と`spawn_tmp_foreground()`は新しいシステムコール番号ではありません．`SYS_SPAWN`の引数個数に`0x100`を付加する方法，または`SYS_SPAWN_TMP`の第2引数を1にする方法で，起動とフォアグラウンドTTY制御権の移譲を不可分に行います．`spawn()`はバックグラウンド起動に使えます．

---

**関連リンク：** [リファレンス目次](README.md) ｜ [English](../../en/reference/SYSCALLS.md)

## `select()`によるI/O多重化

`select()`はTCP/UDPソケット，TTY，パイプの準備状態をカーネルで確認し，必要なら`BLOCKED`にして待機します．タイムアウトは`struct timeval`を使用し，`NULL`は無期限，`{0,0}`は即時確認です．FDは0〜15に限定され，`exceptfds`は非対応です．`fd_set`は16ビットのビット集合です．既存のブロッキング`accept()`・`recv()`・`recvfrom()`も維持します．

```c
fd_set rd;
FD_ZERO(&rd);
FD_SET(server, &rd);
int n = select(server + 1, &rd, NULL, NULL, NULL);
if (n > 0 && FD_ISSET(server, &rd)) {
    int client = accept(server, NULL, NULL);
}
```

書込み準備確認はTCPの送信バッファ空きに基づきますが，送信操作が必ず全データを受け取る保証ではありません．`select()`直後の操作でも`OS_EAGAIN`等の確認が必要です．現時点では通常のRAMファイルなどを`select()`で監視すると`OS_EINVAL`を返します．タイムアウト精度はスケジューラ周期に依存します．
