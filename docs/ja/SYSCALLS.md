# システムコール一覧（現行実装）

この表は `kernel/kernel.c` の `SYS_*` 列挙と `userland/syscalls.c` を照合したものです．番号は内部ABIであり，POSIX互換性を保証しません．

まずCプログラムから使う場合は[アプリ開発](PROGRAMMING.md)を参照してください．このページは，アセンブリ言語やカーネル内部を調べる人向けの**低水準APIの一覧**です．

## 呼び出し規約

- `svc #0`：`r0` = 番号，`r1`–`r3` = 引数，`r0` = 戻り値．
- 番号11と15は予約済みで，通常の呼び出し先ではありません．
- カーネルのディスパッチは `os_svc_dispatch()`，ユーザ側ラッパーは `userland/syscalls.c` にあります．

## 番号一覧

| 番号 | カーネル定数 | ユーザAPI／意味 |
|---:|---|---|
| 1 | `SYS_GETPID` | `getpid` |
| 2 | `SYS_KILL` | `kill` |
| 3 | `SYS_EXIT` | `_exit` |
| 4 | `SYS_SLEEP` | `msleep` |
| 5 | `SYS_YIELD` | `yield` |
| 6 | `SYS_OPEN` | `open` |
| 7 | `SYS_CLOSE` | `close` |
| 8 | `SYS_READ` | `read` |
| 9 | `SYS_WRITE` | `write` |
| 10 | `SYS_IOCTL` | `ioctl` |
| 11 | `SYS_RESERVED_TASK_INFO` | `reserved (task info)` |
| 12 | `SYS_SPAWN` | `spawn` |
| 13 | `SYS_WAITPID` | `waitpid` |
| 14 | `SYS_SET_FG` | `set_foreground` |
| 15 | `SYS_RESERVED_GETARG` | `reserved (getarg)` |
| 16 | `SYS_CONSOLE_READY` | `console_ready` |
| 17 | `SYS_UNLINK` | `unlink` |
| 18 | `SYS_DUP2` | `dup2` |
| 19 | `SYS_TTY_SPAWN` | `tty_spawn` |
| 20 | `SYS_GET_TTY` | `get_tty` |
| 21 | `SYS_LSEEK` | `lseek` |
| 22 | `SYS_CHDIR` | `chdir` |
| 23 | `SYS_GETCWD` | `getcwd` |
| 24 | `SYS_FCNTL` | `fcntl` |
| 25 | `SYS_SPAWN_TMP` | `spawn_tmp` |
| 26 | `SYS_GETDENTS` | `getdents` |
| 27 | `SYS_FLASH_ID` | `flash_id` |
| 28 | `SYS_SOCKET` | `socket` |
| 29 | `SYS_BIND` | `bind_udp / TCP listen adapter` |
| 30 | `SYS_SENDTO` | `sendto_udp` |
| 31 | `SYS_RECVFROM` | `recvfrom_udp` |
| 32 | `SYS_LISTEN_TCP` | `tcp_listen` |
| 33 | `SYS_CONNECT_TCP` | `tcp_connect` |
| 34 | `SYS_TCP_STATUS` | `tcp_status` |
| 35 | `SYS_ACCEPT_TCP` | `tcp_accept` |
| 36 | `SYS_PIPE` | `pipe` |
| 37 | `SYS_DUP` | `dup` |
| 38 | `SYS_PROGRAM_ID` | `program_id` |

## 注意点

- `bind()`，`listen()`，`accept()`，`connect()`，`send()`，`recv()`，`sendto()`，`recvfrom()` は `userland/syscalls.c` のBSD風アダプターです．それぞれが独立したシステムコール番号を持つわけではありません．
- TCP用 `bind()` は内部で待受開始も行い，`listen()` は通常のPOSIXバックログ管理を実装していません．`accept()` の相手アドレス取得も未対応です．
- `pipe()`，`dup()`，`dup2()` はFD操作を提供しますが，POSIXの全機能を実装しているわけではありません．
- アセンブラからは番号を `r0` に設定して `svc #0` を実行します．詳細は [ASM.md](ASM.md) を参照してください．
