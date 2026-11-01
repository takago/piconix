# システムコール一覧（現行実装）

この表は `kernel/kernel.c` の `SYS_*` 列挙と `userland/syscalls.c` を照合したものです．番号は内部ABIであり，POSIX互換性を保証しません．

まずCプログラムから使う場合は[アプリ開発](PROGRAMMING.md)を参照してください．このページは，アセンブリ言語やカーネル内部を調べる人向けの**低水準APIの一覧**です．

## 呼び出し規約

- `svc #0`：`r0` = 番号，`r1`–`r3` = 引数，`r0` = 戻り値．
- 番号11と15は予約済みで，通常の呼び出し先ではありません．
- カーネルのディスパッチは `os_svc_dispatch()`，ユーザ側ラッパーは `userland/syscalls.c` にあります．

## 番号一覧

| 番号 | カーネル定数 | ユーザAPI | 機能 |
|---:|---|---|---|
| 1 | `SYS_GETPID` | `getpid` | 呼び出し元タスクのPIDを取得 |
| 2 | `SYS_KILL` | `kill` | 指定PIDのタスクを終了 |
| 3 | `SYS_EXIT` | `_exit` | 現在のタスクを終了 |
| 4 | `SYS_SLEEP` | `msleep` | 指定ミリ秒だけ実行を休止 |
| 5 | `SYS_YIELD` | `yield` | スケジューラにCPUを譲る |
| 6 | `SYS_OPEN` | `open` | ファイル・デバイスを開いてFDを取得 |
| 7 | `SYS_CLOSE` | `close` | FDを閉じる |
| 8 | `SYS_READ` | `read` | FDからデータを読み取る |
| 9 | `SYS_WRITE` | `write` | FDにデータを書き込む |
| 10 | `SYS_IOCTL` | `ioctl` | デバイス・TTYの設定や制御を行う |
| 11 | `SYS_RESERVED_TASK_INFO` | `reserved (task info)` | 予約番号（呼び出し不可） |
| 12 | `SYS_SPAWN` | `spawn` | 登録済みユーザプログラムを起動 |
| 13 | `SYS_WAITPID` | `waitpid` | 子タスクの終了を待つ |
| 14 | `SYS_SET_FG` | `set_foreground` | TTYの前景タスクを設定 |
| 15 | `SYS_RESERVED_GETARG` | `reserved (getarg)` | 予約番号（呼び出し不可） |
| 16 | `SYS_CONSOLE_READY` | `console_ready` | コンソール接続の準備状態を調べる |
| 17 | `SYS_UNLINK` | `unlink` | 書き込み可能なファイルを削除 |
| 18 | `SYS_DUP2` | `dup2` | 指定番号へFDを複製 |
| 19 | `SYS_TTY_SPAWN` | `tty_spawn` | 指定TTYでシェルまたはgettyを起動 |
| 20 | `SYS_GET_TTY` | `get_tty` | 現在のタスクのTTY識別子を取得 |
| 21 | `SYS_LSEEK` | `lseek` | ファイルの読み書き位置を変更 |
| 22 | `SYS_CHDIR` | `chdir` | カレントディレクトリを変更 |
| 23 | `SYS_GETCWD` | `getcwd` | カレントディレクトリを取得 |
| 24 | `SYS_FCNTL` | `fcntl` | 非ブロッキングなどのFDフラグを取得・設定 |
| 25 | `SYS_SPAWN_TMP` | `spawn_tmp` | /tmp内のThumb実行イメージを起動 |
| 26 | `SYS_GETDENTS` | `getdents` | ディレクトリエントリを取得 |
| 27 | `SYS_FLASH_ID` | `flash_id` | Flashの識別値を取得 |
| 28 | `SYS_SOCKET` | `socket` | TCPまたはUDPソケットを生成 |
| 29 | `SYS_BIND` | `bind_udp / TCP listen adapter` | UDPポートの割当，TCP待受のアダプター処理 |
| 30 | `SYS_SENDTO` | `sendto_udp` | UDPデータグラムを送信 |
| 31 | `SYS_RECVFROM` | `recvfrom_udp` | UDPデータグラムを受信 |
| 32 | `SYS_LISTEN_TCP` | `tcp_listen` | TCPの待受を開始 |
| 33 | `SYS_CONNECT_TCP` | `tcp_connect` | TCPの接続を開始 |
| 34 | `SYS_TCP_STATUS` | `tcp_status` | TCP接続の状態を取得 |
| 35 | `SYS_ACCEPT_TCP` | `tcp_accept` | TCP接続要求を受け付ける |
| 36 | `SYS_PIPE` | `pipe` | パイプと読み書き用FDを生成 |
| 37 | `SYS_DUP` | `dup` | 空いている番号にFDを複製 |
| 38 | `SYS_PROGRAM_ID` | `program_id` | 登録済みプログラム名からIDを取得 |

## 注意点

- `bind()`，`listen()`，`accept()`，`connect()`，`send()`，`recv()`，`sendto()`，`recvfrom()` は `userland/syscalls.c` のBSD風アダプターです．それぞれが独立したシステムコール番号を持つわけではありません．
- TCP用 `bind()` は内部で待受開始も行い，`listen()` は通常のPOSIXバックログ管理を実装していません．`accept()` の相手アドレス取得も未対応です．
- `pipe()`，`dup()`，`dup2()` はFD操作を提供しますが，POSIXの全機能を実装しているわけではありません．
- アセンブラからは番号を `r0` に設定して `svc #0` を実行します．詳細は [ASM.md](ASM.md) を参照してください．
