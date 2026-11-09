# 関数一覧（自動生成）

この文書は `python3 tools/gen_functions.py` でソースから自動生成します．
**手で編集せず，関数直前の日本語コメントを修正して再生成してください．**

- **カーネル**：`kernel/` にあるOS本体の関数．
- **アプリケーション（App）**：`userland/apps/` にあるコマンドやアプリの関数．
- **ライブラリ（Lib）**：`userland/lib/` にある共通Cライブラリとシステムコール呼出しの関数．
- **ホスト用ユーティリティ**：`host/` にあるPC側のPicoNIX操作プログラムの関数（OS本体とは別）．
- **対象外**：`tools/`（ビルド・検証・索引生成），`userland/examples/`（実行例），`userland/fs/`（初期ファイル）．
- C関数定義，Pythonトップレベル関数，アセンブリの関数ラベルを対象とし，ヘッダは**関数定義のみ**を掲載します（宣言・マクロ・型定義は対象外）．
- 関数定義がないファイルは省略します．
- 行番号はこの文書を生成した時点の行番号です．変更後は再生成してください．
- 概要の `—` は関数直前に説明コメントがないことを表します．関数名からの推測は行いません．
- 範囲欄の `static` はCの内部結合，`外部` は外部結合，`モジュール` と `内部名` はPythonの名前付け，`大域` はアセンブリラベルを表します．外部結合は公開APIを意味しません．
- C言語の解析は簡易的な構文走査で，プリプロセッサの条件分岐を評価しません．環境依存の定義は両方載る場合があります．

**掲載関数：829件（概要コメントあり：829件）**

## 分類別目次

- [カーネル](#カーネル)：14ファイル
- [アプリケーション](#アプリケーション)：54ファイル
- [ライブラリ](#ライブラリ)：4ファイル
- [ホスト用ユーティリティ](#ホスト用ユーティリティ)：6ファイル

## カーネル

### `kernel/adc.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`adc_driver_init()`](../../../kernel/adc.c#L13) | 13 | 外部 | ADCと温度センサを初期化し，利用可能な入力ピンをADC機能に切り替える． |
| [`adc_driver_read()`](../../../kernel/adc.c#L22) | 22 | 外部 | 指定ADCチャネルを選択して生の12ビット測定値を返す． |
| [`adc_driver_format()`](../../../kernel/adc.c#L28) | 28 | 外部 | ADC値をテキストへ変換する． |

### `kernel/context.S`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`isr_pendsv()`](../../../kernel/context.S#L12) | 12 | 大域 | PendSV例外でユーザタスクのレジスタとPSPを退避し，スケジューラが選んだタスクを復元する． |
| [`isr_svcall()`](../../../kernel/context.S#L54) | 54 | 大域 | SVC例外で現在のスタックフレームを求め，Cのシステムコールディスパッチャへ渡す． |

### `kernel/fb.c`（8件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`fb_init()`](../../../kernel/fb.c#L21) | 21 | 外部 | 8×8 LEDマトリクスの表示状態と走査位置を初期化する． |
| [`fb_active()`](../../../kernel/fb.c#L24) | 24 | 外部 | フレームバッファ表示が有効なら真を返す． |
| [`fb_tick()`](../../../kernel/fb.c#L27) | 27 | 外部 | タイマ割込みごとに表示行を切り替える． |
| [`fb_ioctl()`](../../../kernel/fb.c#L40) | 40 | 外部 | 表示の有効化・無効化・状態取得を処理する． |
| [`fb_rw()`](../../../kernel/fb.c#L70) | 70 | 外部 | 8バイトの表示ビット列を読み書きする． |
| [`fb_char_copy()`](../../../kernel/fb.c#L98) | 98 | static | 一覧を小さな断片に分け，fd位置から有限長だけ返す． |
| [`fb_char_read()`](../../../kernel/fb.c#L110) | 110 | 外部 | 文字表示デバイスの使用方法と利用可能な図柄を位置指定で返す． |
| [`fb_char_write()`](../../../kernel/fb.c#L124) | 124 | 外部 | ASCII 1文字か記号名を8行へ変換． |

### `kernel/gpio.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`gpio_driver_init()`](../../../kernel/gpio.c#L13) | 13 | 外部 | ボード上のLEDとスイッチを含むGPIOの初期設定を行う． |
| [`gpio_driver_rw()`](../../../kernel/gpio.c#L29) | 29 | 外部 | GPIO値・方向等をデバイスファイル経由で読み書きする． |
| [`gpio_driver_ioctl()`](../../../kernel/gpio.c#L82) | 82 | 外部 | GPIOの入力・出力モード等を変更する． |

### `kernel/i2c.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`i2c_driver_init()`](../../../kernel/i2c.c#L12) | 12 | 外部 | I2C1を初期化し，使用ピンをI2C機能へ設定する． |
| [`i2c_driver_transfer()`](../../../kernel/i2c.c#L21) | 21 | 外部 | アドレス・転送長を検査し，I2Cの書込みと読出しを一つの要求として実行する． |

### `kernel/kernel.c`（150件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`piconix_flash_id()`](../../../kernel/kernel.c#L35) | 35 | 外部 | Flash固有IDの下位32ビットを組み立てる． |
| [`vfs_fixed_path_id()`](../../../kernel/kernel.c#L112) | 112 | static | 固定デバイス／仮想ファイルの絶対パスを内部IDに変換する． |
| [`vfs_directory_id()`](../../../kernel/kernel.c#L133) | 133 | static | 固定ディレクトリ名をIDへ変換する． |
| [`vfs_directory_path()`](../../../kernel/kernel.c#L141) | 141 | static | 固定ディレクトリIDから絶対パスを求める． |
| [`vfs_fixed_id_path()`](../../../kernel/kernel.c#L146) | 146 | static | 登録済みのデバイス・仮想ファイルIDをパスに戻す． |
| [`vfs_device_count()`](../../../kernel/kernel.c#L157) | 157 | static | 固定デバイス登録表の項目数を返す． |
| [`vfs_device_path()`](../../../kernel/kernel.c#L160) | 160 | static | 固定デバイス表の指定番号に対応するパスを返す． |
| [`vfs_gpio_id()`](../../../kernel/kernel.c#L165) | 165 | static | GPIOのファイル名を内部IDへ変換する． |
| [`pipe_end()`](../../../kernel/kernel.c#L227) | 227 | static | デバイスIDがパイプの読取端・書込端に属するか判定する． |
| [`pipe_for_dev()`](../../../kernel/kernel.c#L230) | 230 | static | パイプ端点のデバイスIDから，対応する固定長パイプ管理領域を求める． |
| [`pipe_is_writer()`](../../../kernel/kernel.c#L233) | 233 | static | パイプ端点のIDから書込側かどうかを判定する． |
| [`pipe_ref()`](../../../kernel/kernel.c#L236) | 236 | static | パイプ端点の参照数をdeltaだけ増減する． |
| [`pipe_transfer()`](../../../kernel/kernel.c#L246) | 246 | static | 固定256バイトのリングで部分読み書きを行う． |
| [`rand32()`](../../../kernel/kernel.c#L289) | 289 | static | xorshiftによる32ビット擬似乱数を更新して返す． |
| [`__attribute__()`](../../../kernel/kernel.c#L346) | 346 | static | 32バイトのトレースイベントを16 KiBリングへ記録する． |
| [`foreground_child()`](../../../kernel/kernel.c#L392) | 392 | static | 指定PIDの子を現在のTTYの前景へ移す． |
| [`tty_owner_pid()`](../../../kernel/kernel.c#L421) | 421 | static | TTYを占有するプロセスを求める． |
| [`take_exit()`](../../../kernel/kernel.c#L426) | 426 | static | 親PIDに紐づく終了通知を一件取り出す． |
| [`fd_set()`](../../../kernel/kernel.c#L439) | 439 | static | 一つのFDに関連する内部ID，位置，フラグ，対象PIDをまとめて設定する． |
| [`udp_fd_slot()`](../../../kernel/kernel.c#L449) | 449 | static | UDP FDの所有者と管理スロットを検証する． |
| [`tcp_fd_slot()`](../../../kernel/kernel.c#L457) | 457 | static | タスク内のTCP用FDを検証し，対応するネットワーク管理スロットを返す． |
| [`tcp_other_fd()`](../../../kernel/kernel.c#L465) | 465 | static | 同一TCP管理スロットを参照する別のFDを探し，重複参照の後始末に使う． |
| [`fd_is_socket()`](../../../kernel/kernel.c#L473) | 473 | static | ソケットFDは一つのlwIP資源を所有するため，パイプと異なりdup()の対象外とする． |
| [`fd_release()`](../../../kernel/kernel.c#L479) | 479 | static | FDが保持する資源を解放する． |
| [`fd_inherit()`](../../../kernel/kernel.c#L508) | 508 | static | FDを子／別番号へ複製する． |
| [`fd_copy()`](../../../kernel/kernel.c#L519) | 519 | static | 同一タスク内でFDの参照先と状態を複製する． |
| [`task_active()`](../../../kernel/kernel.c#L524) | 524 | static | 終了済みのタスクスロットは直ちに再利用できる． |
| [`tty_foreground_survivor()`](../../../kernel/kernel.c#L529) | 529 | static | パイプ段のうち現在生存する前景プロセスを探す． |
| [`finish_task()`](../../../kernel/kernel.c#L542) | 542 | static | タスク終了時に通知，FD，通信資源，TTYの前景・モードを後始末する． |
| [`task_unblock()`](../../../kernel/kernel.c#L606) | 606 | static | 中断中のSVCの保存フレームへ戻り値を書き込み，待機情報を消してREADYにする． |
| [`wake_pipes()`](../../../kernel/kernel.c#L619) | 619 | static | ブロック中のパイプread/writeを再試行する． |
| [`wake_sockets()`](../../../kernel/kernel.c#L640) | 640 | static | lwIPの受信状態をスケジューラから確認してソケット待機を解除する． |
| [`select_ready()`](../../../kernel/kernel.c#L698) | 698 | static | 選択したFDの読書き可能性を調べる． |
| [`wake_selects()`](../../../kernel/kernel.c#L747) | 747 | static | SVCで停止したselectをスケジューラから再検査する． |
| [`wake_waiters()`](../../../kernel/kernel.c#L763) | 763 | static | 子の終了を待っていた親を起こす． |
| [`free_program_slot()`](../../../kernel/kernel.c#L784) | 784 | static | 未使用または終了済みの24 KiBユーザスロットを探す． |
| [`os_register_ram_image()`](../../../kernel/kernel.c#L794) | 794 | 外部 | Flash上のPICイメージとゼロ初期化範囲を登録する． |
| [`tty_mode_reset()`](../../../kernel/kernel.c#L810) | 810 | static | TTYを標準の行入力・エコー・改行変換へ戻し，未完了入力も初期化する． |
| [`pty_ref()`](../../../kernel/kernel.c#L818) | 818 | static | Master/SlaveのFD参照数を追う． |
| [`vt_clear()`](../../../kernel/kernel.c#L838) | 838 | static | 仮想画面の最小ANSI解釈． |
| [`vt_scroll()`](../../../kernel/kernel.c#L843) | 843 | static | 最下行を空けながら仮想画面を1行スクロールする． |
| [`vt_lf()`](../../../kernel/kernel.c#L850) | 850 | static | 改行時に下端を超えた場合は1行スクロールする． |
| [`vt_cursor_report()`](../../../kernel/kernel.c#L854) | 854 | static | ESC[6nへの応答はPTYの入力キューに入れる． |
| [`vt_arg()`](../../../kernel/kernel.c#L861) | 861 | static | ANSIのパラメータが省略された場合に既定値を返す． |
| [`pty_screen_byte()`](../../../kernel/kernel.c#L865) | 865 | static | PTYの出力を最小限のANSI状態機械で画面へ描画する． |
| [`pty_screen_read()`](../../../kernel/kernel.c#L924) | 924 | static | 1行80文字+LFを24行． |
| [`pty_key_write()`](../../../kernel/kernel.c#L941) | 941 | static | 名前による特殊キー入力． |
| [`pty_output_put()`](../../../kernel/kernel.c#L970) | 970 | static | Masterが読み取るSlave出力を，1024バイトのリングへ欠落させずに追加する． |
| [`pty_output_read()`](../../../kernel/kernel.c#L979) | 979 | static | Masterがreadで取り出す出力リング． |
| [`tty_raw_enqueue()`](../../../kernel/kernel.c#L991) | 991 | static | 非カノニカル入力を小さな固定キューへ積む． |
| [`tty_enqueue()`](../../../kernel/kernel.c#L1010) | 1010 | static | 編集済み行を受信キューへ移す． |
| [`tty_dequeue()`](../../../kernel/kernel.c#L1019) | 1019 | static | TTYキューから最大nバイト返す． |
| [`uart_pop()`](../../../kernel/kernel.c#L1034) | 1034 | static | 指定TTYの受信リングから1バイト取り出す． |
| [`uart1_read()`](../../../kernel/kernel.c#L1043) | 1043 | static | UART1をTTY所有状態に応じて読み分ける． |
| [`uart_push()`](../../../kernel/kernel.c#L1054) | 1054 | static | 受信リングへ1バイト追加する． |
| [`console_queue()`](../../../kernel/kernel.c#L1063) | 1063 | static | USB CDCへの送信データを固定長リングへ積む． |
| [`console_tx_poll()`](../../../kernel/kernel.c#L1076) | 1076 | static | アイドルタスク側でUSB送信リングを少量ずつ空にする． |
| [`tty_put()`](../../../kernel/kernel.c#L1092) | 1092 | static | USB CDCコンソールの送信キューへ1文字を渡す． |
| [`tty_put_dev()`](../../../kernel/kernel.c#L1097) | 1097 | static | TTY番号に応じてUSB CDC，UART1，Wi-Fi端末へ1文字出力する． |
| [`tty_tx_char()`](../../../kernel/kernel.c#L1107) | 1107 | static | TTY固有の出力先へ1文字を書き込む． |
| [`pty_slave_write()`](../../../kernel/kernel.c#L1132) | 1132 | static | PTY Slaveの出力は文字欠落を避けるため，空き容量分だけ転送する． |
| [`tty_line_columns()`](../../../kernel/kernel.c#L1147) | 1147 | static | 正規モードでTABは0x09のまま保持し，エコー表示だけ4桁タブ位置へ展開する． |
| [`tty_foreground_interrupt()`](../../../kernel/kernel.c#L1156) | 1156 | static | Ctrl+Cで同じTTYの前景グループを終了させる． |
| [`tty_input_char()`](../../../kernel/kernel.c#L1171) | 1171 | static | TTYの入力モードに従い1文字を処理する． |
| [`pty_master_rw()`](../../../kernel/kernel.c#L1211) | 1211 | static | PTY Masterは出力を読み，Slaveへの入力をTTY行規律へ渡す． |
| [`wifi_session_stop()`](../../../kernel/kernel.c#L1245) | 1245 | static | 切断されたTelnetセッションのタスクとTTY状態を破棄する． |
| [`wifi_session_poll()`](../../../kernel/kernel.c#L1261) | 1261 | static | Telnetの接続状態・世代番号を監視して専用シェルを作る． |
| [`os_console_poll()`](../../../kernel/kernel.c#L1299) | 1299 | 外部 | アイドル側からUSB／Telnet入力，送信キュー，保留中のUART Ctrl+Cを処理する． |
| [`tty_rx_ready()`](../../../kernel/kernel.c#L1327) | 1327 | static | 共有TTY受信経路． |
| [`tty_rx_read()`](../../../kernel/kernel.c#L1336) | 1336 | static | 対象TTYの受信キューから読み取る． |
| [`wake_readers()`](../../../kernel/kernel.c#L1343) | 1343 | static | TTY入力待ちのタスクを，受信データと前景所有条件が揃った場合に再開させる． |
| [`uart1_irq_handler()`](../../../kernel/kernel.c#L1369) | 1369 | static | UART1の受信割込みを処理し，入力文字をTTYバッファへ蓄える． |
| [`pend()`](../../../kernel/kernel.c#L1380) | 1380 | static | PendSVを保留状態にする． |
| [`task_return()`](../../../kernel/kernel.c#L1383) | 1383 | static | タスクの入口関数がreturnした場合の終了処理へ誘導する． |
| [`os_init()`](../../../kernel/kernel.c#L1386) | 1386 | 外部 | タスク表，FD，TTY，固定メモリ管理と割込みを初期化する． |
| [`allocate_pid()`](../../../kernel/kernel.c#L1415) | 1415 | static | トレースの8ビットPIDに合わせて1〜255を循環使用する． |
| [`spawn_in_slot()`](../../../kernel/kernel.c#L1438) | 1438 | static | 指定スロットにPICイメージを複写し，argc/argvと初期レジスタフレームを配置する． |
| [`os_init_idle()`](../../../kernel/kernel.c#L1509) | 1509 | 外部 | アイドルタスク専用スタックと初期例外フレームを用意する． |
| [`os_idle_stack_used_bytes()`](../../../kernel/kernel.c#L1528) | 1528 | 外部 | スタックに埋めた0xA5の残量から使用量を推定する． |
| [`os_idle_stack_capacity_bytes()`](../../../kernel/kernel.c#L1536) | 1536 | 外部 | アイドルタスク専用スタックの確保サイズをバイト単位で返す． |
| [`os_spawn_named()`](../../../kernel/kernel.c#L1539) | 1539 | 外部 | スケジューラ開始前に指定の入口関数をタスクとして登録する． |
| [`os_register_program()`](../../../kernel/kernel.c#L1550) | 1550 | 外部 | プログラムID・名前・入口を起動前の表へ登録する． |
| [`os_start_program()`](../../../kernel/kernel.c#L1557) | 1557 | 外部 | 登録済みプログラムを起動前の空きスロットへ配置してPIDを返す． |
| [`os_spawn()`](../../../kernel/kernel.c#L1565) | 1565 | 外部 | 起動前に無名タスクを登録する簡易入口． |
| [`isr_systick()`](../../../kernel/kernel.c#L1568) | 1568 | 外部 | 1 ms周期の時刻更新と睡眠解除を行う． |
| [`os_schedule()`](../../../kernel/kernel.c#L1590) | 1590 | 外部 | PendSVが保存した旧PSPを受け，次のタスクのPSPを返す． |
| [`os_start()`](../../../kernel/kernel.c#L1650) | 1650 | 外部 | SysTickを1 msに設定し，最初のPendSVでタスク実行を開始する． |
| [`slot_from_pid()`](../../../kernel/kernel.c#L1664) | 1664 | static | 有効なPIDを持つタスクのスロット番号を探す． |
| [`in_task_range()`](../../../kernel/kernel.c#L1672) | 1672 | static | ユーザポインタpからnバイトが現在のスロット内に収まるか検査する． |
| [`path_match()`](../../../kernel/kernel.c#L1681) | 1681 | static | この関数へ渡すパスはカーネル側で正規化済みであり，ユーザポインタの検査は呼出側で終えている． |
| [`tmp_payload()`](../../../kernel/kernel.c#L1695) | 1695 | static | RAMファイル番号から固定サイズのデータ領域先頭アドレスを計算する． |
| [`tmp_name()`](../../../kernel/kernel.c#L1701) | 1701 | static | /tmpまたは/etc直下の名前を検証して内部形式へ変換する． |
| [`tmp_init()`](../../../kernel/kernel.c#L1721) | 1721 | static | Flashに埋め込まれた初期ファイルを起動後のRAMファイル領域へ一度だけ展開する． |
| [`tmp_lookup()`](../../../kernel/kernel.c#L1738) | 1738 | static | /tmpにあるRAMファイルを名前で検索し，該当番号または-1を返す． |
| [`free_fd()`](../../../kernel/kernel.c#L1747) | 1747 | static | ユーザがopenするFDは3以降． |
| [`proc_device()`](../../../kernel/kernel.c#L1755) | 1755 | static | デバイスIDが/procの仮想ファイルを表すか判定する． |
| [`tmp_open()`](../../../kernel/kernel.c#L1765) | 1765 | static | 固定11個のRAMファイルスロットから対象を探し，必要なら作成する． |
| [`tmp_unlink()`](../../../kernel/kernel.c#L1795) | 1795 | static | RAMファイルを削除する． |
| [`tmp_rw()`](../../../kernel/kernel.c#L1813) | 1813 | static | ファイルの現在位置を使って読み書きする． |
| [`absolute_path()`](../../../kernel/kernel.c#L1842) | 1842 | static | タスクごとのcwdを使って相対パスを正規化する． |
| [`valid_directory()`](../../../kernel/kernel.c#L1889) | 1889 | static | VFSが公開する固定ディレクトリか調べる． |
| [`vdir_nth()`](../../../kernel/kernel.c#L1895) | 1895 | static | 仮想ディレクトリのnth番目の項目を計算する． |
| [`vdir_entry_device_id()`](../../../kernel/kernel.c#L1989) | 1989 | static | ディレクトリエントリの表示IDを，プログラム／デバイス／RAMファイルの識別子に対応させる． |
| [`vfs_getdents()`](../../../kernel/kernel.c#L2027) | 2027 | static | ディレクトリFDの現在位置から固定長エントリを返す． |
| [`vfs_open()`](../../../kernel/kernel.c#L2042) | 2042 | static | 正規化済み絶対パスをデバイス／ディレクトリ／RAMファイルへ振り分け，FDを割り当てる． |
| [`device_rw_rand()`](../../../kernel/kernel.c#L2132) | 2132 | static | /dev/randomの読出し． |
| [`proc_emit()`](../../../kernel/kernel.c#L2153) | 2153 | static | /proc出力の位置と要求バイト数を考慮して，文字列の一部分だけをコピーする． |
| [`proc_tasks_read()`](../../../kernel/kernel.c#L2170) | 2170 | static | 生存タスクの概要を/proc/tasks形式へ逐次生成する． |
| [`proc_fb_read()`](../../../kernel/kernel.c#L2196) | 2196 | static | /procの8×8表示状況をテキストで返す． |
| [`fd_path()`](../../../kernel/kernel.c#L2220) | 2220 | static | FDの表示名はカーネルが管理するデバイスIDから求める． |
| [`proc_task_read()`](../../../kernel/kernel.c#L2270) | 2270 | static | 指定スロットのタスク詳細とFDを表示用テキストへ整形する． |
| [`proc_ramfiles_read()`](../../../kernel/kernel.c#L2320) | 2320 | static | 使用中RAMファイルの実データアドレスと名前を列挙する． |
| [`device_rw_proc()`](../../../kernel/kernel.c#L2343) | 2343 | static | /procの読み取りを担当する． |
| [`device_rw_tty()`](../../../kernel/kernel.c#L2410) | 2410 | static | TTYに対するread/writeを実装する． |
| [`wake_ptys()`](../../../kernel/kernel.c#L2459) | 2459 | static | PTYのブロッキングread/writeを，Masterの出力回収後などに再試行する． |
| [`device_rw_pwm()`](../../../kernel/kernel.c#L2484) | 2484 | static | /dev/pwmの文字列読書き． |
| [`device_rw_adc()`](../../../kernel/kernel.c#L2512) | 2512 | static | ADCを数値文字列として読む． |
| [`vfs_rw()`](../../../kernel/kernel.c#L2537) | 2537 | static | read/writeの共通入口． |
| [`device_ioctl_tty()`](../../../kernel/kernel.c#L2779) | 2779 | static | 端末の所有権と行入力・エコー・改行変換を管理する． |
| [`device_ioctl_gpio_status()`](../../../kernel/kernel.c#L2835) | 2835 | static | GPIO値のFDから，各ピンの状態を問い合わせられる． |
| [`device_ioctl()`](../../../kernel/kernel.c#L2863) | 2863 | static | ioctlの共通入口． |
| [`os_svc_dispatch()`](../../../kernel/kernel.c#L2926) | 2926 | 外部 | SVC例外で積まれたr0〜r3を取り出し，番号ごとの処理結果をr0へ返す． |
| [`call()`](../../../kernel/kernel.c#L3463) | 3463 | static | SVCディスパッチャへ渡す引数は明示的にレジスタへ割り当てる． |
| [`os_waitpid()`](../../../kernel/kernel.c#L3473) | 3473 | 外部 | SVCでwaitpidを実行するカーネル側ラッパー． |
| [`os_set_foreground()`](../../../kernel/kernel.c#L3478) | 3478 | 外部 | SVCで指定PIDをTTYの前景プロセスへ設定する． |
| [`os_spawn_program()`](../../../kernel/kernel.c#L3481) | 3481 | 外部 | SVCで登録済みのプログラムIDを起動する． |
| [`os_getpid()`](../../../kernel/kernel.c#L3484) | 3484 | 外部 | SVCを呼び出し，現在実行中のPIDを取得する． |
| [`os_get_tty()`](../../../kernel/kernel.c#L3487) | 3487 | 外部 | SVCを呼び出し，現在のタスクが所属するTTY番号を取得する． |
| [`os_kill()`](../../../kernel/kernel.c#L3490) | 3490 | 外部 | SVCで指定PIDのタスクの終了を要求する． |
| [`os_exit()`](../../../kernel/kernel.c#L3493) | 3493 | 外部 | SVCで現在のタスクを終了する． |
| [`os_msleep()`](../../../kernel/kernel.c#L3496) | 3496 | 外部 | SVCで指定ミリ秒だけタスクを睡眠状態へ移す． |
| [`os_yield()`](../../../kernel/kernel.c#L3499) | 3499 | 外部 | SVCで自発的にCPU実行権を譲る． |
| [`os_open()`](../../../kernel/kernel.c#L3502) | 3502 | 外部 | SVCでVFSのパスを開き，FDまたはエラー値を返す． |
| [`os_close()`](../../../kernel/kernel.c#L3505) | 3505 | 外部 | SVCで指定FDを閉じ，必要な端点参照を解放する． |
| [`os_read()`](../../../kernel/kernel.c#L3508) | 3508 | 外部 | SVCでFDから最大nバイトを読み取る． |
| [`os_write()`](../../../kernel/kernel.c#L3511) | 3511 | 外部 | SVCでFDに最大nバイトを書き込む． |
| [`os_ioctl()`](../../../kernel/kernel.c#L3514) | 3514 | 外部 | SVCでデバイス固有操作を要求する． |
| [`os_fcntl()`](../../../kernel/kernel.c#L3517) | 3517 | 外部 | SVCでFDフラグ等を取得・変更する． |
| [`os_pipe()`](../../../kernel/kernel.c#L3520) | 3520 | 外部 | SVCでパイプを作成し，読取端と書込端のFDを配列へ返す． |
| [`os_dup()`](../../../kernel/kernel.c#L3523) | 3523 | 外部 | SVCでFDを複製し，共有する端点への参照を増やす． |
| [`os_dup2()`](../../../kernel/kernel.c#L3526) | 3526 | 外部 | SVCでFDを指定番号へ複製する． |
| [`os_unlink()`](../../../kernel/kernel.c#L3529) | 3529 | 外部 | SVCでRAMファイルの指定パスを削除する． |
| [`os_socket()`](../../../kernel/kernel.c#L3532) | 3532 | 外部 | TCPクライアント・サーバとUDP通信を提供するソケット関連システムコール． |
| [`os_tcp_listen()`](../../../kernel/kernel.c#L3535) | 3535 | 外部 | SVCで指定FDをTCPの待受に設定する． |
| [`os_tcp_connect()`](../../../kernel/kernel.c#L3538) | 3538 | 外部 | SVCで指定FDのTCP接続を開始する． |
| [`os_tcp_status()`](../../../kernel/kernel.c#L3541) | 3541 | 外部 | SVCでTCP接続の進行・確立・異常を問い合わせる． |
| [`os_tcp_accept()`](../../../kernel/kernel.c#L3544) | 3544 | 外部 | SVCで待受FDから確立済みの接続を受け取り，新しいFDを得る． |
| [`__attribute__()`](../../../kernel/kernel.c#L3554) | 3554 | static | コンパイラの関数入口・出口フックを受け，教材用の選抜関数だけ記録する． |
| [`__attribute__()`](../../../kernel/kernel.c#L3646) | 3646 | 外部 | -finstrument-functions が生成する入口通知． |
| [`__attribute__()`](../../../kernel/kernel.c#L3653) | 3653 | 外部 | -finstrument-functions が生成する出口通知． |

### `kernel/main.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`idle_task()`](../../../kernel/main.c#L13) | 13 | static | ユーザタスクが動かない間も，通信とUSB端末を継続処理する． |
| [`register_image()`](../../../kernel/main.c#L78) | 78 | static | Flash内のバイナリ範囲をプログラムIDに紐づける． |
| [`main()`](../../../kernel/main.c#L87) | 87 | 外部 | OSと各ユーザプログラムを登録し，初期タスクを配置してスケジューラへ制御を渡す． |

### `kernel/net.c`（27件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`wifi_cache_mac()`](../../../kernel/net.c#L37) | 37 | static | CYW43から取得したMACアドレスをキャッシュする． |
| [`wifi_mac_text()`](../../../kernel/net.c#L48) | 48 | static | 以前取得したMACアドレスを表示文字列にする． |
| [`net_poll_in_progress()`](../../../kernel/net.c#L72) | 72 | 外部 | lwIPのポーリング処理中か返す． |
| [`sta_rssi_clear()`](../../../kernel/net.c#L90) | 90 | static | STAのRSSIキャッシュを無効化し，次回の取得を許可する． |
| [`sta_rssi_poll()`](../../../kernel/net.c#L96) | 96 | static | STA接続時のRSSIを低頻度に取得し，診断表示にはキャッシュを使う． |
| [`sta_rssi_text()`](../../../kernel/net.c#L111) | 111 | static | RSSIを使った電波強度の推定値であり，通信品質の実測値ではない． |
| [`sta_mdns_remove()`](../../../kernel/net.c#L131) | 131 | static | STA側のmDNS登録を取り消し，再接続時の重複登録を避ける． |
| [`sta_mdns_ready()`](../../../kernel/net.c#L140) | 140 | static | IPアドレス確立後のmDNS提供状態を管理する． |
| [`sta_link_name()`](../../../kernel/net.c#L174) | 174 | static | CYW43のリンク状態コードを診断表示用の文字列に変換する． |
| [`sta_reset_association()`](../../../kernel/net.c#L188) | 188 | static | STA接続の状態と関連する情報を初期化して再接続可能にする． |
| [`sta_begin_connect()`](../../../kernel/net.c#L200) | 200 | static | STA接続を非同期に開始する． |
| [`sta_poll()`](../../../kernel/net.c#L215) | 215 | static | STAのリンク・DHCP取得と再接続の状態遷移を進める． |
| [`sta_start_services()`](../../../kernel/net.c#L293) | 293 | static | STAのリンク確立後にネットワークサービスとmDNS等を起動する． |
| [`dhcp_expire_offer()`](../../../kernel/net.c#L332) | 332 | static | 未完了のDHCP交渉が次の端末の接続を永久に妨げないようにする． |
| [`dhcp_recv()`](../../../kernel/net.c#L339) | 339 | static | AP側のDHCP/BOOTP要求を検査する． |
| [`ap_dhcp_server_start()`](../../../kernel/net.c#L429) | 429 | static | APモードのDHCPサービスを開始し，固定長の情報でアドレスを配布する． |
| [`net_init()`](../../../kernel/net.c#L450) | 450 | 外部 | Wi-Fi/LwIPを初期化する． |
| [`dhcp_send_pending()`](../../../kernel/net.c#L507) | 507 | static | cyw43_arch_pollとlwIPコールバックの外側から呼ぶ． |
| [`net_poll()`](../../../kernel/net.c#L540) | 540 | 外部 | アイドルタスクから無線・TCP/IP・保留送信・ループバックを順次処理する． |
| [`net_init()`](../../../kernel/net.c#L584) | 584 | 外部 | Wi-Fi/LwIPを初期化する． |
| [`net_poll()`](../../../kernel/net.c#L587) | 587 | 外部 | アイドルタスクから無線・TCP/IP・保留送信・ループバックを順次処理する． |
| [`net_poll_in_progress()`](../../../kernel/net.c#L590) | 590 | 外部 | 通常Picoビルドでもスケジューラから呼ばれる． |
| [`net_memory_info()`](../../../kernel/net.c#L594) | 594 | 外部 | lwIPヒープやプールの診断情報をテキスト化する． |
| [`net_wifi_info()`](../../../kernel/net.c#L638) | 638 | 外部 | /proc/net/wifi用の状態をテキストへ整形する． |
| [`net_wifi_info()`](../../../kernel/net.c#L693) | 693 | 外部 | /proc/net/wifi用の状態をテキストへ整形する． |
| [`net_wifi_request_restart()`](../../../kernel/net.c#L707) | 707 | 外部 | 再起動要求のフラグを立てる． |
| [`net_diag_info()`](../../../kernel/net.c#L719) | 719 | 外部 | AP/ARPの診断データを取得する． |

### `kernel/net_capture.c`（23件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`be16()`](../../../kernel/net_capture.c#L38) | 38 | static | パケット上のビッグエンディアン16ビット値を取り出す． |
| [`observe()`](../../../kernel/net_capture.c#L41) | 41 | static | ネットワークパケットの要約を固定長履歴へ記録する． |
| [`piconix_packet_input()`](../../../kernel/net_capture.c#L86) | 86 | 外部 | IPv4入力パケットを観測し，通常のlwIP受信処理は継続させる． |
| [`observed_output()`](../../../kernel/net_capture.c#L93) | 93 | static | lwIPの送信フックでパケットを記録し，元の送信関数を呼ぶ． |
| [`net_capture_attach()`](../../../kernel/net_capture.c#L102) | 102 | 外部 | 観測フックをネットワークIFへ接続する． |
| [`net_packet_serial()`](../../../kernel/net_capture.c#L114) | 114 | 外部 | パケット観測リングへ記録した通算番号を返す． |
| [`net_packet_reader()`](../../../kernel/net_capture.c#L117) | 117 | 外部 | 観測デバイスの読取利用数を加減する． |
| [`net_packet_read()`](../../../kernel/net_capture.c#L120) | 120 | 外部 | パケット記録を指定位置から読み出し，連続した観測に使う． |
| [`echo_received()`](../../../kernel/net_capture.c#L131) | 131 | static | ICMP Echo応答を検査し，待機している要求と対応する場合に結果を保存する． |
| [`net_capture_init()`](../../../kernel/net_capture.c#L156) | 156 | 外部 | ICMP Echo応答を受け取るraw PCBを初期化する． |
| [`net_echo_write()`](../../../kernel/net_capture.c#L163) | 163 | 外部 | ICMP Echo要求をネットワーク側へ送る． |
| [`net_echo_read()`](../../../kernel/net_capture.c#L172) | 172 | 外部 | 受信済みEcho応答を取り出す． |
| [`net_echo_close()`](../../../kernel/net_capture.c#L181) | 181 | 外部 | 未完了のEcho要求・応答状態を消去する． |
| [`net_capture_poll()`](../../../kernel/net_capture.c#L184) | 184 | 外部 | Echo待機のタイムアウトを確認し，応答状態を更新する． |
| [`net_capture_init()`](../../../kernel/net_capture.c#L221) | 221 | 外部 | ICMP Echo応答を受け取るraw PCBを初期化する． |
| [`net_capture_poll()`](../../../kernel/net_capture.c#L224) | 224 | 外部 | Echo待機のタイムアウトを確認し，応答状態を更新する． |
| [`net_capture_attach()`](../../../kernel/net_capture.c#L227) | 227 | 外部 | 観測フックをネットワークIFへ接続する． |
| [`net_packet_serial()`](../../../kernel/net_capture.c#L230) | 230 | 外部 | パケット観測リングへ記録した通算番号を返す． |
| [`net_packet_reader()`](../../../kernel/net_capture.c#L233) | 233 | 外部 | 観測デバイスの読取利用数を加減する． |
| [`net_packet_read()`](../../../kernel/net_capture.c#L236) | 236 | 外部 | パケット記録を指定位置から読み出し，連続した観測に使う． |
| [`net_echo_write()`](../../../kernel/net_capture.c#L239) | 239 | 外部 | ICMP Echo要求をネットワーク側へ送る． |
| [`net_echo_read()`](../../../kernel/net_capture.c#L242) | 242 | 外部 | 受信済みEcho応答を取り出す． |
| [`net_echo_close()`](../../../kernel/net_capture.c#L245) | 245 | 外部 | 未完了のEcho要求・応答状態を消去する． |

### `kernel/net_service_remote_devctl.c`（8件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`net_device_request()`](../../../kernel/net_service_remote_devctl.c#L13) | 13 | static | UDPで届いたデバイス操作要求を検査し，対応するGPIO等の操作へ変換する． |
| [`device_recv()`](../../../kernel/net_service_remote_devctl.c#L130) | 130 | static | ネットワークコールバックで要求を受け取る． |
| [`device_start()`](../../../kernel/net_service_remote_devctl.c#L143) | 143 | static | UDP 2000番のデバイス制御受信口を初期化し，コールバックを設定する． |
| [`device_process_pending()`](../../../kernel/net_service_remote_devctl.c#L155) | 155 | static | 保留したデバイス要求をアイドル文脈で処理する． |
| [`net_service_remote_devctl_init()`](../../../kernel/net_service_remote_devctl.c#L172) | 172 | 外部 | デバイス制御サービスを初期化する． |
| [`net_service_remote_devctl_poll()`](../../../kernel/net_service_remote_devctl.c#L175) | 175 | 外部 | 受信済みのデバイス制御要求を処理する． |
| [`net_service_remote_devctl_init()`](../../../kernel/net_service_remote_devctl.c#L178) | 178 | 外部 | デバイス制御サービスを初期化する． |
| [`net_service_remote_devctl_poll()`](../../../kernel/net_service_remote_devctl.c#L181) | 181 | 外部 | 受信済みのデバイス制御要求を処理する． |

### `kernel/net_service_telnet.c`（20件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`queue_count()`](../../../kernel/net_service_telnet.c#L30) | 30 | static | Telnetのリングキュー内に存在するバイト数を計算する． |
| [`queue_space()`](../../../kernel/net_service_telnet.c#L35) | 35 | static | キュー末尾を1つ空けたうえで書き込めるバイト数を返す． |
| [`net_queues_reset()`](../../../kernel/net_service_telnet.c#L40) | 40 | static | Telnetの送受信キューを空にし，新規接続へ備える． |
| [`net_terminal_connected()`](../../../kernel/net_service_telnet.c#L45) | 45 | 外部 | Telnetクライアントの接続有無を返す． |
| [`net_terminal_epoch()`](../../../kernel/net_service_telnet.c#L48) | 48 | 外部 | 端末接続の世代番号を返す． |
| [`net_terminal_read()`](../../../kernel/net_service_telnet.c#L51) | 51 | 外部 | 受信キューから指定した最大バイト数を取り出す． |
| [`net_terminal_write()`](../../../kernel/net_service_telnet.c#L64) | 64 | 外部 | TTY送信バイトを固定長キューに積み，積めた量を返す． |
| [`net_terminal_flush()`](../../../kernel/net_service_telnet.c#L91) | 91 | static | ポーリング文脈からネットワークへ送信キューを流す． |
| [`client_error()`](../../../kernel/net_service_telnet.c#L123) | 123 | static | lwIPが通知したTCP異常を受け，破棄済みPCBへの参照を外す． |
| [`net_receive_byte()`](../../../kernel/net_service_telnet.c#L132) | 132 | static | Telnetの制御列と通常入力を分ける． |
| [`client_recv()`](../../../kernel/net_service_telnet.c#L166) | 166 | static | TCPコールバックで受信を固定キューへ入れる． |
| [`client_accept()`](../../../kernel/net_service_telnet.c#L205) | 205 | static | 単一のTelnetクライアントを受け入れ，TTY用の新しいセッション番号を与える． |
| [`net_service_telnet_init()`](../../../kernel/net_service_telnet.c#L232) | 232 | 外部 | TCP 23番のリスナを用意する． |
| [`net_service_telnet_poll()`](../../../kernel/net_service_telnet.c#L251) | 251 | 外部 | 保留されたTelnet送信と切断処理をアイドル側で進める． |
| [`net_service_telnet_init()`](../../../kernel/net_service_telnet.c#L254) | 254 | 外部 | TCP 23番のリスナを用意する． |
| [`net_service_telnet_poll()`](../../../kernel/net_service_telnet.c#L257) | 257 | 外部 | 保留されたTelnet送信と切断処理をアイドル側で進める． |
| [`net_terminal_connected()`](../../../kernel/net_service_telnet.c#L260) | 260 | 外部 | Telnetクライアントの接続有無を返す． |
| [`net_terminal_epoch()`](../../../kernel/net_service_telnet.c#L263) | 263 | 外部 | 端末接続の世代番号を返す． |
| [`net_terminal_read()`](../../../kernel/net_service_telnet.c#L266) | 266 | 外部 | 受信キューから指定した最大バイト数を取り出す． |
| [`net_terminal_write()`](../../../kernel/net_service_telnet.c#L269) | 269 | 外部 | TTY送信バイトを固定長キューに積み，積めた量を返す． |

### `kernel/net_socket.c`（55件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`udp_slot_at()`](../../../kernel/net_socket.c#L28) | 28 | static | UDPソケットの番号を検証して対応する固定スロットを返す． |
| [`udp_received()`](../../../kernel/net_socket.c#L33) | 33 | static | lwIPコールバックは登録時に渡されたスロットの受信領域だけを書き換える． |
| [`net_socket_udp_read_ready()`](../../../kernel/net_socket.c#L48) | 48 | 外部 | 指定ソケットが読み取り可能か調べる． |
| [`net_socket_udp_bind()`](../../../kernel/net_socket.c#L54) | 54 | 外部 | 指定スロットのUDP待受ポートを登録する． |
| [`net_socket_udp_recv()`](../../../kernel/net_socket.c#L70) | 70 | 外部 | スロット固有の保留データグラムを取り出す． |
| [`net_socket_udp_send()`](../../../kernel/net_socket.c#L84) | 84 | 外部 | UDPクライアントは初回送信時に自身のPCBと送信元ポートを取得する． |
| [`net_socket_udp_close()`](../../../kernel/net_socket.c#L109) | 109 | 外部 | ソケットを閉じるときは対象スロットだけを破棄する． |
| [`slot_at()`](../../../kernel/net_socket.c#L135) | 135 | static | IDが有効な範囲にある場合だけ固定TCPスロットのポインタを返す． |
| [`net_socket_tcp_alloc()`](../../../kernel/net_socket.c#L140) | 140 | 外部 | 固定TCP管理スロットを確保する． |
| [`socket_tcp_connected_cb()`](../../../kernel/net_socket.c#L151) | 151 | static | 非同期TCP接続の完了を受け，接続状態とエラーを更新する． |
| [`socket_tcp_err()`](../../../kernel/net_socket.c#L161) | 161 | static | lwIPがTCP PCBを破棄したときに管理スロットの参照を解除する． |
| [`socket_tcp_recv()`](../../../kernel/net_socket.c#L173) | 173 | static | TCP受信コールバック． |
| [`socket_tcp_accept()`](../../../kernel/net_socket.c#L192) | 192 | static | 受け付けたTCP接続を管理スロットに登録し，accept側へ新しい接続の到着を通知する． |
| [`net_socket_tcp_listen()`](../../../kernel/net_socket.c#L211) | 211 | 外部 | 指定スロットでTCP待受を始める． |
| [`net_socket_tcp_connect()`](../../../kernel/net_socket.c#L229) | 229 | 外部 | TCP接続を非同期に開始する． |
| [`net_socket_tcp_accept_peek()`](../../../kernel/net_socket.c#L253) | 253 | 外部 | selectは受付フラグを消さず，accept()に引き継ぐ． |
| [`net_socket_tcp_recv_ready()`](../../../kernel/net_socket.c#L258) | 258 | 外部 | TCP受信データまたは切断・エラーの通知があるか調べる． |
| [`net_socket_tcp_write_ready()`](../../../kernel/net_socket.c#L263) | 263 | 外部 | TCP送信バッファに空きがあり書き込み可能か調べる． |
| [`net_socket_tcp_accept_ready()`](../../../kernel/net_socket.c#L270) | 270 | 外部 | 待受スロットに受け取り可能な接続があるか確認する． |
| [`net_socket_tcp_listening()`](../../../kernel/net_socket.c#L279) | 279 | 外部 | 指定TCPスロットが待受状態なら真を返す． |
| [`net_socket_tcp_close_listener()`](../../../kernel/net_socket.c#L282) | 282 | 外部 | TCP待受PCBを閉じる． |
| [`socket_tcp_closing_poll()`](../../../kernel/net_socket.c#L294) | 294 | static | 保留したtcp_closeはTCPタイマーコールバックからだけ再試行する． |
| [`socket_tcp_closing_recv()`](../../../kernel/net_socket.c#L301) | 301 | static | 終了処理中に到着した受信バッファを破棄して受信窓を進める． |
| [`net_socket_tcp_close_client()`](../../../kernel/net_socket.c#L309) | 309 | 外部 | アプリから切り離したTCP接続を閉じる． |
| [`net_socket_tcp_connected()`](../../../kernel/net_socket.c#L330) | 330 | 外部 | 指定TCPスロットの接続が確立済みか返す． |
| [`net_socket_tcp_status()`](../../../kernel/net_socket.c#L333) | 333 | 外部 | TCP接続の状態を返す． |
| [`net_socket_tcp_read()`](../../../kernel/net_socket.c#L345) | 345 | 外部 | 固定受信キューから最大capacityバイトをコピーする． |
| [`net_socket_tcp_write()`](../../../kernel/net_socket.c#L362) | 362 | 外部 | 送信バッファとMSSに収まる範囲だけをlwIPへ渡す． |
| [`net_socket_tcp_shutdown_write()`](../../../kernel/net_socket.c#L383) | 383 | 外部 | TCPの送信方向だけを終了し，受信方向は維持する． |
| [`net_socket_tcp_close()`](../../../kernel/net_socket.c#L394) | 394 | 外部 | 指定TCPスロットのクライアントと待受を閉じ，管理スロットを解放する． |
| [`net_socket_tcp_diagnostics()`](../../../kernel/net_socket.c#L403) | 403 | 外部 | /proc/net/memoryから参照する累積TCPエラー統計． |
| [`net_socket_local_port()`](../../../kernel/net_socket.c#L417) | 417 | 外部 | UDPまたはTCPの指定ソケットに割り当てられたローカルポートを返す． |
| [`net_socket_udp_bind()`](../../../kernel/net_socket.c#L432) | 432 | 外部 | UDP待受ポートをlwIPへ登録する． |
| [`net_socket_udp_read_ready()`](../../../kernel/net_socket.c#L434) | 434 | 外部 | UDPの受信データがあるか確認する． |
| [`net_socket_udp_recv()`](../../../kernel/net_socket.c#L437) | 437 | 外部 | 保持中の一つのデータグラムを取り出す． |
| [`net_socket_udp_send()`](../../../kernel/net_socket.c#L442) | 442 | 外部 | 宛先IPv4とポートを指定してUDPデータグラムを送る． |
| [`net_socket_udp_close()`](../../../kernel/net_socket.c#L447) | 447 | 外部 | UDPソケットを解放し，受信状態をリセットする． |
| [`net_socket_tcp_alloc()`](../../../kernel/net_socket.c#L450) | 450 | 外部 | 固定TCP管理スロットを確保する． |
| [`net_socket_tcp_listen()`](../../../kernel/net_socket.c#L453) | 453 | 外部 | 指定スロットでTCP待受を始める． |
| [`net_socket_tcp_accept_ready()`](../../../kernel/net_socket.c#L456) | 456 | 外部 | 待受スロットに受け取り可能な接続があるか確認する． |
| [`net_socket_tcp_accept_peek()`](../../../kernel/net_socket.c#L458) | 458 | 外部 | TCP待受スロットに受け付け可能な接続があるか確認する． |
| [`net_socket_tcp_recv_ready()`](../../../kernel/net_socket.c#L460) | 460 | 外部 | TCP受信データまたは切断・エラーの通知があるか調べる． |
| [`net_socket_tcp_write_ready()`](../../../kernel/net_socket.c#L462) | 462 | 外部 | TCP送信バッファに空きがあり書き込み可能か調べる． |
| [`net_socket_tcp_listening()`](../../../kernel/net_socket.c#L465) | 465 | 外部 | 指定TCPスロットが待受状態なら真を返す． |
| [`net_socket_tcp_close_listener()`](../../../kernel/net_socket.c#L468) | 468 | 外部 | TCP待受PCBを閉じる． |
| [`net_socket_tcp_close_client()`](../../../kernel/net_socket.c#L471) | 471 | 外部 | アプリから切り離したTCP接続を閉じる． |
| [`net_socket_tcp_connected()`](../../../kernel/net_socket.c#L474) | 474 | 外部 | 指定TCPスロットの接続が確立済みか返す． |
| [`net_socket_tcp_status()`](../../../kernel/net_socket.c#L477) | 477 | 外部 | TCP接続の状態を返す． |
| [`net_socket_tcp_connect()`](../../../kernel/net_socket.c#L480) | 480 | 外部 | TCP接続を非同期に開始する． |
| [`net_socket_tcp_read()`](../../../kernel/net_socket.c#L483) | 483 | 外部 | 固定受信キューから最大capacityバイトをコピーする． |
| [`net_socket_tcp_write()`](../../../kernel/net_socket.c#L486) | 486 | 外部 | 送信バッファとMSSに収まる範囲だけをlwIPへ渡す． |
| [`net_socket_tcp_shutdown_write()`](../../../kernel/net_socket.c#L489) | 489 | 外部 | TCPの送信方向だけを終了し，受信方向は維持する． |
| [`net_socket_tcp_close()`](../../../kernel/net_socket.c#L492) | 492 | 外部 | 指定TCPスロットのクライアントと待受を閉じ，管理スロットを解放する． |
| [`net_socket_tcp_diagnostics()`](../../../kernel/net_socket.c#L495) | 495 | 外部 | 非Wi-Fi版ではTCP資源を確保しない． |
| [`net_socket_local_port()`](../../../kernel/net_socket.c#L498) | 498 | 外部 | UDPまたはTCPの指定ソケットに割り当てられたローカルポートを返す． |

### `kernel/pins.h`（6件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`pin_mode()`](../../../kernel/pins.h#L122) | 122 | static | ボードごとの固定ピン設定表から，GPIO／ADC／PWM等の利用モードを返す． |
| [`pin_reserved()`](../../../kernel/pins.h#L130) | 130 | static | 指定ピンが汎用GPIOとして利用可能なら真を返す． |
| [`pin_gpio()`](../../../kernel/pins.h#L139) | 139 | static | 指定ピンが予約されておらずGPIOモードであるかを判定する． |
| [`pin_adc()`](../../../kernel/pins.h#L142) | 142 | static | 指定チャネルに対応するGPIOがADC用なら真を返す． |
| [`pin_pwm()`](../../../kernel/pins.h#L145) | 145 | static | GP18／GP19がPWM用に割り当てられているか確認する． |
| [`pin_any_pwm()`](../../../kernel/pins.h#L148) | 148 | static | PWMに割り当てたピンが1本以上あるか返す． |

### `kernel/pwm.c`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`apply_pwm()`](../../../kernel/pwm.c#L17) | 17 | static | 周波数とデューティ比をRP2040のPWMレジスタへ反映する． |
| [`pwm_driver_init()`](../../../kernel/pwm.c#L42) | 42 | 外部 | PWMピンの機能を設定し，既定の周波数とデューティ比を適用する． |
| [`pwm_driver_get()`](../../../kernel/pwm.c#L51) | 51 | 外部 | 0は周波数，1・2は各チャネルのデューティ比を返す． |
| [`pwm_driver_set()`](../../../kernel/pwm.c#L54) | 54 | 外部 | 周波数またはデューティ比を検証して設定する． |
| [`pwm_parse_value()`](../../../kernel/pwm.c#L63) | 63 | 外部 | ローカルVFS書込みとUDPデバイス制御で共用する処理． |

## アプリケーション

### `userland/apps/asm.c`（12件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`space()`](../../../userland/apps/asm.c#L23) | 23 | static | 入力文字列の先頭にある空白・タブを読み飛ばし，次の文字位置を返す． |
| [`end()`](../../../userland/apps/asm.c#L26) | 26 | static | 行末側の空白・タブ・復帰文字を削除し，字句解析を簡単にする． |
| [`number()`](../../../userland/apps/asm.c#L29) | 29 | static | 10進・16進の即値を解析する． |
| [`reg()`](../../../userland/apps/asm.c#L42) | 42 | static | レジスタ表記r0〜r7を検証し，番号または-1を返す． |
| [`registers()`](../../../userland/apps/asm.c#L48) | 48 | static | push／popのレジスタ集合をビットマスクへ変換する． |
| [`memory_operand()`](../../../userland/apps/asm.c#L66) | 66 | static | [rN, 即値]形式のメモリオペランドから基底レジスタとオフセットを求める． |
| [`label()`](../../../userland/apps/asm.c#L78) | 78 | static | 1パス目で登録したラベル名を探し，コード内の相対位置を返す． |
| [`emit16()`](../../../userland/apps/asm.c#L81) | 81 | static | Thumb命令を16ビット単位で出力する． |
| [`byte()`](../../../userland/apps/asm.c#L84) | 84 | static | 1バイトをコードへ追加する． |
| [`compile()`](../../../userland/apps/asm.c#L87) | 87 | static | ソースを1パスずつ解析する． |
| [`write_all()`](../../../userland/apps/asm.c#L220) | 220 | static | FDへ全バイトを書き終えるまでwriteを繰り返す． |
| [`main()`](../../../userland/apps/asm.c#L223) | 223 | 外部 | Thumb-1アセンブリのファイルを読み，2パスで変換して実行用イメージを出力する． |

### `userland/apps/bh1750.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`transfer()`](../../../userland/apps/bh1750.c#L18) | 18 | static | /dev/i2c/1を使い，BH1750へ指定バイトを書いて測定値を受け取る． |
| [`main()`](../../../userland/apps/bh1750.c#L39) | 39 | 外部 | BH1750照度センサをI2Cで操作し，取得した照度値を表示する． |

### `userland/apps/blink.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/blink.c#L10) | 10 | 外部 | GPIOを出力に設定する． |

### `userland/apps/cat.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`output()`](../../../userland/apps/cat.c#L11) | 11 | static | 標準出力への書込みを繰り返し，部分書込みと一時的な待機を処理する． |
| [`numbered_output()`](../../../userland/apps/cat.c#L24) | 24 | static | 32バイト単位のreadをまたいでも，空行を含む行頭位置を保持する． |
| [`display()`](../../../userland/apps/cat.c#L44) | 44 | static | 読み取ったデータ片を表示する． |
| [`main()`](../../../userland/apps/cat.c#L50) | 50 | 外部 | 指定ファイルまたは標準入力の内容を順番に表示する． |

### `userland/apps/cc.c`（37件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`letter()`](../../../userland/apps/cc.c#L24) | 24 | static | 1=char, 4=int |
| [`digit()`](../../../userland/apps/cc.c#L26) | 26 | static | 入力文字が十進数字であるか判定する． |
| [`ws()`](../../../userland/apps/cc.c#L28) | 28 | static | 空白とC形式のコメントを読み飛ばす． |
| [`word()`](../../../userland/apps/cc.c#L41) | 41 | static | 指定した予約語を単語境界も確認して読み取る． |
| [`token()`](../../../userland/apps/cc.c#L47) | 47 | static | 次の1文字が指定した記号なら消費する． |
| [`need()`](../../../userland/apps/cc.c#L49) | 49 | static | 指定した記号を要求し，見つからなければエラーにする． |
| [`integer()`](../../../userland/apps/cc.c#L51) | 51 | static | 符号付き十進整数を範囲検査しながら読み取る． |
| [`emit()`](../../../userland/apps/cc.c#L69) | 69 | static | 生成中のThumbアセンブリを出力領域へ追加する． |
| [`imm()`](../../../userland/apps/cc.c#L96) | 96 | static | 8ビット即値を使う命令を生成する． |
| [`regimm()`](../../../userland/apps/cc.c#L101) | 101 | static | レジスタ番号と8ビット即値を指定して命令を生成する． |
| [`constant()`](../../../userland/apps/cc.c#L108) | 108 | static | Materialize signed 32-bit constants with Thumb-1 movs/adds only. |
| [`regmove()`](../../../userland/apps/cc.c#L120) | 120 | static | レジスタ間転送命令を生成する． |
| [`branch()`](../../../userland/apps/cc.c#L124) | 124 | static | 指定ラベルへの分岐命令を生成する． |
| [`mark()`](../../../userland/apps/cc.c#L128) | 128 | static | 分岐先ラベルを出力する． |
| [`identifier()`](../../../userland/apps/cc.c#L130) | 130 | static | C識別子を読み取って名前を保存する． |
| [`character()`](../../../userland/apps/cc.c#L137) | 137 | static | 文字リテラルと一部のエスケープを読み取る． |
| [`variable()`](../../../userland/apps/cc.c#L145) | 145 | static | 変数名からローカル変数表の位置を求める． |
| [`local_access()`](../../../userland/apps/cc.c#L151) | 151 | static | r4 is a callee-saved frame pointer: stack temporaries must not shift references to locals and parameters within the fixed 256-byte frame. |
| [`syscall_buffer()`](../../../userland/apps/cc.c#L175) | 175 | static | Evaluate arguments left-to-right. |
| [`address()`](../../../userland/apps/cc.c#L189) | 189 | static | ローカル変数のフレーム内アドレスを生成する． |
| [`array_address()`](../../../userland/apps/cc.c#L195) | 195 | static | Calculate array-element address, for both real arrays and pointers. |
| [`incdec()`](../../../userland/apps/cc.c#L205) | 205 | static | Increment/decrement an int local. |
| [`step_token()`](../../../userland/apps/cc.c#L214) | 214 | static | Parse ++ or -- as a single token, never as two unary operators. |
| [`load_element()`](../../../userland/apps/cc.c#L221) | 221 | static | 配列またはポインタ要素を読み出す命令を生成する． |
| [`function_call()`](../../../userland/apps/cc.c#L227) | 227 | static | AAPCS: up to four int parameters in r0-r3, result in r0. |
| [`primary()`](../../../userland/apps/cc.c#L263) | 263 | static | 式の基本要素を解析して評価コードを生成する． |
| [`unary()`](../../../userland/apps/cc.c#L283) | 283 | static | 単項演算子とオペランドを解析する． |
| [`multiply()`](../../../userland/apps/cc.c#L304) | 304 | static | 乗除算など乗法優先順位の式を解析する． |
| [`additive()`](../../../userland/apps/cc.c#L321) | 321 | static | 加減算など加法優先順位の式を解析する． |
| [`expression()`](../../../userland/apps/cc.c#L335) | 335 | static | 式全体を解析して結果のコードを生成する． |
| [`condition()`](../../../userland/apps/cc.c#L337) | 337 | static | 条件式の評価と分岐を生成する． |
| [`finish()`](../../../userland/apps/cc.c#L356) | 356 | static | 関数末尾の復帰処理を生成する． |
| [`block()`](../../../userland/apps/cc.c#L361) | 361 | static | 中括弧で囲まれた文の列を解析する． |
| [`statement()`](../../../userland/apps/cc.c#L365) | 365 | static | 代入や制御構文などの文を解析する． |
| [`compile()`](../../../userland/apps/cc.c#L486) | 486 | static | 入力Cソースを解析してThumbアセンブリへ変換する． |
| [`assembly_path()`](../../../userland/apps/cc.c#L545) | 545 | static | Keep the assembly next to the source: foo.c -> foo.s, /tmp/foo.c -> /tmp/foo.s. |
| [`main()`](../../../userland/apps/cc.c#L555) | 555 | 外部 | 引数とファイルを処理しCコンパイルを実行する． |

### `userland/apps/cp.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`path()`](../../../userland/apps/cp.c#L11) | 11 | static | 自分自身をtruncateしないよう，コピー元とコピー先の相対パスを正規化する． |
| [`main()`](../../../userland/apps/cp.c#L47) | 47 | 外部 | 指定したファイルの内容を別ファイルへ複写する． |

### `userland/apps/dd.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`number()`](../../../userland/apps/dd.c#L12) | 12 | static | コマンド引数の10進数または0x付き16進数を解析する． |
| [`usage()`](../../../userland/apps/dd.c#L21) | 21 | static | ddで利用可能な入出力指定と転送オプションを表示する． |
| [`main()`](../../../userland/apps/dd.c#L24) | 24 | 外部 | 指定した入力FDから出力FDへデータをブロック単位でコピーする． |

### `userland/apps/echo.c`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`flush()`](../../../userland/apps/echo.c#L14) | 14 | static | 蓄積した標準出力を部分書込みに対応して送出する． |
| [`emit()`](../../../userland/apps/echo.c#L27) | 27 | static | エスケープ展開後の1バイトを蓄積する． |
| [`digit()`](../../../userland/apps/echo.c#L34) | 34 | static | エスケープの16進数字を数値へ変換する． |
| [`output()`](../../../userland/apps/echo.c#L41) | 41 | static | エスケープ指定を考慮し，echoの引数文字列を出力する． |
| [`main()`](../../../userland/apps/echo.c#L72) | 72 | 外部 | 引数を空白区切りで表示し，オプションに応じて改行・エスケープを扱う． |

### `userland/apps/edit.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/edit.c#L12) | 12 | 外部 | 共通エディタ処理を使って編集対象を開き，キー入力で編集して保存する． |

### `userland/apps/editor_common.h`（15件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`width()`](../../../userland/apps/editor_common.h#L22) | 22 | static | 文字の画面表示幅を返す． |
| [`line_start()`](../../../userland/apps/editor_common.h#L25) | 25 | static | 指定位置を含む行の先頭オフセットを探す． |
| [`line_end()`](../../../userland/apps/editor_common.h#L28) | 28 | static | 指定位置を含む行の改行手前のオフセットを探す． |
| [`line_no()`](../../../userland/apps/editor_common.h#L31) | 31 | static | テキスト先頭から指定位置までの改行数を数え，行番号を返す． |
| [`col_no()`](../../../userland/apps/editor_common.h#L34) | 34 | static | 指定位置の表示桁をタブ展開込みで求める． |
| [`position_col()`](../../../userland/apps/editor_common.h#L37) | 37 | static | 行頭から指定表示桁まで進め，対応する文字オフセットを返す． |
| [`insert()`](../../../userland/apps/editor_common.h#L40) | 40 | static | カーソル位置に文字を挿入する． |
| [`erase()`](../../../userland/apps/editor_common.h#L43) | 43 | static | カーソル直前の文字を削除し，後続バイトを左へ詰める． |
| [`del()`](../../../userland/apps/editor_common.h#L46) | 46 | static | カーソル位置の文字を削除し，後続バイトを左へ詰める． |
| [`save()`](../../../userland/apps/editor_common.h#L49) | 49 | static | 編集バッファを指定RAMファイルへ保存する． |
| [`measure_screen()`](../../../userland/apps/editor_common.h#L52) | 52 | static | 端末サイズが極端に大きくても，描画出力は固定上限に収める． |
| [`paint_row()`](../../../userland/apps/editor_common.h#L61) | 61 | static | EDITOR_GUTTERはeditで行番号用の4桁，viでは0となる． |
| [`draw()`](../../../userland/apps/editor_common.h#L85) | 85 | static | 表示モードは0=全体，1=変更行，2=カーソルと状態のみ． |
| [`editor_normalize_path()`](../../../userland/apps/editor_common.h#L115) | 115 | static | 書込み可能なRAMパスを検査する前に「.」「..」を正規化する． |
| [`editor_start()`](../../../userland/apps/editor_common.h#L138) | 138 | static | パス解決・2 KiBまでの読出し・TTY設定を共通化する． |

### `userland/apps/fbctl.c`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`same()`](../../../userland/apps/fbctl.c#L12) | 12 | static | 二つのコマンド文字列が等しいか判定する． |
| [`pattern()`](../../../userland/apps/fbctl.c#L15) | 15 | static | 8バイトのフレームバッファパターンをFDへ書き込む． |
| [`status()`](../../../userland/apps/fbctl.c#L18) | 18 | static | cat /proc/fbと同じカーネル生成レポートを表示する． |
| [`usage()`](../../../userland/apps/fbctl.c#L39) | 39 | static | LED表示の有効化・消灯・テスト操作の使用方法を表示する． |
| [`main()`](../../../userland/apps/fbctl.c#L51) | 51 | 外部 | /dev/fbを介して8×8 LED表示の状態確認や操作を行う． |

### `userland/apps/fbed.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`draw()`](../../../userland/apps/fbed.c#L11) | 11 | static | 編集中の8×8 LED画面とカーソルを端末へ描画する． |
| [`main()`](../../../userland/apps/fbed.c#L27) | 27 | 外部 | 端末から8×8の点灯パターンを編集し，フレームバッファへ反映する． |

### `userland/apps/fd.c`（21件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`reset_view_index()`](../../../userland/apps/fd.c#L42) | 42 | static | ファイル表示用の疎な行索引を先頭位置で初期化する． |
| [`index_line()`](../../../userland/apps/fd.c#L48) | 48 | static | 表示範囲内のファイル内容を行ごとに索引化する． |
| [`clipped()`](../../../userland/apps/fd.c#L59) | 59 | static | 指定幅以内で文字列を切り出し，必ず終端文字を付ける． |
| [`join()`](../../../userland/apps/fd.c#L66) | 66 | static | ディレクトリとエントリ名から絶対パスを組み立てる． |
| [`parent_dir()`](../../../userland/apps/fd.c#L77) | 77 | static | 現在の閲覧パスを親ディレクトリへ移動する． |
| [`name_matches()`](../../../userland/apps/fd.c#L85) | 85 | static | ASCII大文字・小文字を区別せず，名前の一部との一致を判定する． |
| [`lower_ascii()`](../../../userland/apps/fd.c#L102) | 102 | static | ASCII大文字・小文字を区別せずに文字を比較する． |
| [`find_text_line()`](../../../userland/apps/fd.c#L109) | 109 | static | テキストを逐次走査して検索する． |
| [`go_to_match()`](../../../userland/apps/fd.c#L150) | 150 | static | ビューア内の検索結果へ移動する． |
| [`directory_load()`](../../../userland/apps/fd.c#L158) | 158 | static | getdentsでディレクトリを列挙し，表示用の項目を保持する． |
| [`refresh_dir()`](../../../userland/apps/fd.c#L183) | 183 | static | 現在ディレクトリのエントリをVFSから読み直し，選択位置を更新する． |
| [`draw_dir()`](../../../userland/apps/fd.c#L190) | 190 | static | ファイル一覧と選択位置を端末画面へ表示する． |
| [`safe_view()`](../../../userland/apps/fd.c#L219) | 219 | static | 既知のテキスト系ファイルだけを閲覧する． |
| [`view_load()`](../../../userland/apps/fd.c#L225) | 225 | static | ファイルの対象範囲を読み取り，現在のページ表示を準備する． |
| [`print_matched_line()`](../../../userland/apps/fd.c#L292) | 292 | static | 表示行の検索語だけを反転表示する． |
| [`paint_view()`](../../../userland/apps/fd.c#L311) | 311 | static | 現在のファイルページを端末へ描画する． |
| [`view_hash()`](../../../userland/apps/fd.c#L343) | 343 | static | 画面本文のFNV-1a指紋． |
| [`refresh_view()`](../../../userland/apps/fd.c#L353) | 353 | static | ファイルの表示領域を読み直す． |
| [`edit_selected()`](../../../userland/apps/fd.c#L366) | 366 | static | 選択したRAMファイルに対する編集コマンドを呼び出す． |
| [`key_event()`](../../../userland/apps/fd.c#L398) | 398 | static | キー入力からディレクトリ移動・閲覧等の操作を選択する． |
| [`main()`](../../../userland/apps/fd.c#L413) | 413 | 外部 | 端末を設定してファイルブラウザの操作ループを実行する． |

### `userland/apps/find.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`match()`](../../../userland/apps/find.c#L8) | 8 | static | ファイル名を*と?の簡易ワイルドカードに照合する． |
| [`visit()`](../../../userland/apps/find.c#L19) | 19 | static | 列挙したエントリを表示し，ディレクトリなら深さ制限内で再帰探索する． |
| [`descend()`](../../../userland/apps/find.c#L27) | 27 | static | 指定ディレクトリを巡回する． |
| [`main()`](../../../userland/apps/find.c#L34) | 34 | 外部 | 開始位置からVFSをたどり，条件に合うパスを表示する． |

### `userland/apps/gpio.c`（11件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`available()`](../../../userland/apps/gpio.c#L16) | 16 | static | 画面に表示するGPIO番号の範囲を確認し，未公開ピンを除外する． |
| [`at()`](../../../userland/apps/gpio.c#L19) | 19 | static | 行と列から，表形式表示に対応するGPIO番号を計算する． |
| [`editable()`](../../../userland/apps/gpio.c#L22) | 22 | static | GPIO状態がFREE・IN・OUTのいずれかで，操作を許可できるか判定する． |
| [`query()`](../../../userland/apps/gpio.c#L27) | 27 | static | カーネルのGPIO状態取得ioctlを呼び，指定ピンの状態値を得る． |
| [`owner()`](../../../userland/apps/gpio.c#L30) | 30 | static | 予約済みGPIO状態コードから使用機能の表示名を返す． |
| [`pin_ioctl()`](../../../userland/apps/gpio.c#L43) | 43 | static | 対象GPIOのデバイスを開いてioctlを実行し，FDを閉じる． |
| [`set_level()`](../../../userland/apps/gpio.c#L54) | 54 | static | GPIOデバイスに0／1を書き込んで出力レベルを設定する． |
| [`cell()`](../../../userland/apps/gpio.c#L66) | 66 | static | 1ピン分の所有者・方向・レベルを表示し，選択ピンを反転表示する． |
| [`draw()`](../../../userland/apps/gpio.c#L85) | 85 | static | GPIOを格子状に描画し，利用可否と現在の選択を示す． |
| [`navigate()`](../../../userland/apps/gpio.c#L106) | 106 | static | 表示グリッド順に移動し，利用不可ピンを飛ばし，既存の所有関係を変更しない． |
| [`main()`](../../../userland/apps/gpio.c#L119) | 119 | 外部 | GPIO状態を端末で対話的に表示・変更する． |

### `userland/apps/grep.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`emit()`](../../../userland/apps/grep.c#L11) | 11 | static | 見つかった行を標準出力へ転送する． |
| [`match()`](../../../userland/apps/grep.c#L22) | 22 | static | 指定した検索語が対象の行に含まれるかを調べる． |
| [`main()`](../../../userland/apps/grep.c#L36) | 36 | 外部 | 標準入力やファイルから条件に一致する行を抽出する． |

### `userland/apps/hello.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/hello.c#L8) | 8 | 外部 | PicoNIXの動作確認用メッセージを表示する． |

### `userland/apps/help.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/help.c#L9) | 9 | 外部 | 登録コマンドと利用方法の簡易案内を表示する． |

### `userland/apps/hexdump.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`number()`](../../../userland/apps/hexdump.c#L13) | 13 | static | オフセット表示などに用いる数値引数を解析する． |
| [`main()`](../../../userland/apps/hexdump.c#L26) | 26 | 外部 | 入力ファイルを16進数と文字の対応が分かる形式で表示する． |

### `userland/apps/httpd.c`（11件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`sendall()`](../../../userland/apps/httpd.c#L11) | 11 | static | TCP接続への送信を完了するまで繰り返す． |
| [`same()`](../../../userland/apps/httpd.c#L21) | 21 | static | HTTPのパスや操作名を文字列一致で判定する． |
| [`response()`](../../../userland/apps/httpd.c#L24) | 24 | static | HTTP/1.0の状態行・ヘッダ・本文を送る． |
| [`text_reply()`](../../../userland/apps/httpd.c#L33) | 33 | static | UTF-8プレーンテキストのHTTP応答を生成する． |
| [`hexval()`](../../../userland/apps/httpd.c#L36) | 36 | static | 16進数1桁を0〜15へ変換する． |
| [`pwm_path()`](../../../userland/apps/httpd.c#L39) | 39 | static | Web APIのPWMパスをVFSの/dev/pwmパスへ変換する． |
| [`api()`](../../../userland/apps/httpd.c#L47) | 47 | static | フレームバッファとPWM用HTTP APIを解析してデバイス操作へ振り分ける． |
| [`content_length()`](../../../userland/apps/httpd.c#L115) | 115 | static | ヘッダを一行ずつ検査し，曖昧・不正な長さを拒否する． |
| [`recv_timed()`](../../../userland/apps/httpd.c#L142) | 142 | static | ノンブロッキング通信ではOS_EAGAINは一時的な状態，0はTCP EOFである． |
| [`handle()`](../../../userland/apps/httpd.c#L153) | 153 | static | 固定長要求バッファでGETと小さなPOSTだけを扱い，chunked転送は実装しない． |
| [`main()`](../../../userland/apps/httpd.c#L216) | 216 | 外部 | TCP待受と要求処理を繰り返す． |

### `userland/apps/i2cctl.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`number()`](../../../userland/apps/i2cctl.c#L12) | 12 | static | I2Cアドレス・転送値の数値引数を解析する． |
| [`usage()`](../../../userland/apps/i2cctl.c#L20) | 20 | static | I2C制御コマンドの指定形式を表示する． |
| [`main()`](../../../userland/apps/i2cctl.c#L26) | 26 | 外部 | /dev/i2c/1のioctlを使い，指定デバイスへ書込み・読出しを行う． |

### `userland/apps/ip.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`field()`](../../../userland/apps/ip.c#L9) | 9 | static | /proc/net/wifiの行から指定キーの値を取り出し，固定長領域に格納する． |
| [`main()`](../../../userland/apps/ip.c#L28) | 28 | 外部 | ループバックとWi-Fiインターフェースのアドレス情報を読み取り専用で表示する． |

### `userland/apps/kill.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/kill.c#L8) | 8 | 外部 | 指定PIDに終了要求を送り，結果を表示する． |

### `userland/apps/kitesys.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`display()`](../../../userland/apps/kitesys.c#L24) | 24 | static | 数字フォントを8×8表示用の行データへ変換し，フレームバッファへ送る． |
| [`pressed()`](../../../userland/apps/kitesys.c#L32) | 32 | static | Lowで押下を検出し，30 msデバウンス後の安定した押下を一回だけ返す． |
| [`pwm_value()`](../../../userland/apps/kitesys.c#L51) | 51 | static | PWMデバイスへ数値文字列を送り，書込みの成否を返す． |
| [`main()`](../../../userland/apps/kitesys.c#L59) | 59 | 外部 | ボタンでLED表示位置を動かし，PWMブザーも鳴らす統合動作テストを行う． |

### `userland/apps/ls.c`（9件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`eq()`](../../../userland/apps/ls.c#L8) | 8 | static | 二つのファイル名・オプション文字列が等しいか判定する． |
| [`write_all()`](../../../userland/apps/ls.c#L11) | 11 | static | 標準出力への全バイトの書込みを行う． |
| [`put()`](../../../userland/apps/ls.c#L14) | 14 | static | 表示文字列を標準出力へ出力する． |
| [`number()`](../../../userland/apps/ls.c#L17) | 17 | static | 数値を表示用の文字列へ変換する． |
| [`resolve()`](../../../userland/apps/ls.c#L20) | 20 | static | 相対パスを現在ディレクトリからのパスへ変換する． |
| [`entry()`](../../../userland/apps/ls.c#L58) | 58 | static | デバイスはデバイスID，/binはプログラムID，RAMファイルはスロットIDを表示する． |
| [`file_entry()`](../../../userland/apps/ls.c#L83) | 83 | static | 親ディレクトリからファイル属性を取得する． |
| [`ls()`](../../../userland/apps/ls.c#L107) | 107 | static | ディレクトリなら内容を，通常ファイルならそのファイル自身を表示する． |
| [`main()`](../../../userland/apps/ls.c#L140) | 140 | 外部 | 指定パスの一覧を表示し，オプションに従って表示形式を切り替える． |

### `userland/apps/lsof.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`number()`](../../../userland/apps/lsof.c#L11) | 11 | static | /procの数値フィールドを読み，次の文字位置へ進める． |
| [`show_task()`](../../../userland/apps/lsof.c#L14) | 14 | static | /proc/task/PIDを解析して対象プロセスのFD一覧を表示する． |
| [`main()`](../../../userland/apps/lsof.c#L55) | 55 | 外部 | 指定PIDまたは全プロセスについて，開いているFDの内容を表示する． |

### `userland/apps/mb.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`draw()`](../../../userland/apps/mb.c#L18) | 18 | static | 複数行の月面バギーを固定位置に描く． |
| [`main()`](../../../userland/apps/mb.c#L67) | 67 | 外部 | 端末を非カノニカル・非ブロック入力に変更して月面バギーゲームを動かす． |

### `userland/apps/mcpd.c`（28件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`space()`](../../../userland/apps/mcpd.c#L32) | 32 | static | JSON文字列とコンテナを走査し，指定された値の終端を探す． |
| [`json_string_end()`](../../../userland/apps/mcpd.c#L39) | 39 | static | JSON文字列の終端位置を検証しながら取得する． |
| [`json_end()`](../../../userland/apps/mcpd.c#L57) | 57 | static | JSON値の終端位置を調べる． |
| [`member()`](../../../userland/apps/mcpd.c#L98) | 98 | static | 単純なJSONオブジェクトから，直下のキーのみを探す． |
| [`json_equals()`](../../../userland/apps/mcpd.c#L131) | 131 | static | JSON文字列が指定文字列と一致するか調べる． |
| [`append_escaped()`](../../../userland/apps/mcpd.c#L138) | 138 | static | 小さなASCIIテキストをJSON文字列に安全に入れる． |
| [`read_device()`](../../../userland/apps/mcpd.c#L168) | 168 | static | デバイスから現在の情報を読み取る． |
| [`send_all()`](../../../userland/apps/mcpd.c#L182) | 182 | static | 非ブロッキング送信は部分成功と一時的な送信キュー満杯を区別する． |
| [`http_reply()`](../../../userland/apps/mcpd.c#L206) | 206 | static | HTTPレスポンスを組み立てて返す． |
| [`header_value()`](../../../userland/apps/mcpd.c#L223) | 223 | static | ヘッダは比較に用いる名前と値だけを取得し，重複を拒否する． |
| [`rpc_error()`](../../../userland/apps/mcpd.c#L274) | 274 | static | JSON-RPCエラー応答を作成する． |
| [`rpc_result()`](../../../userland/apps/mcpd.c#L281) | 281 | static | JSON-RPC正常応答を作成する． |
| [`command_argument()`](../../../userland/apps/mcpd.c#L287) | 287 | static | JSON文字列からコマンドを読み取る． |
| [`decode_file_text()`](../../../userland/apps/mcpd.c#L306) | 306 | static | JSONの文字列を逐次復号する． |
| [`file_path_argument()`](../../../userland/apps/mcpd.c#L354) | 354 | static | 書込先は/tmp直下の短いファイル名だけに制限する． |
| [`send_chunk()`](../../../userland/apps/mcpd.c#L371) | 371 | static | 1チャンクを一つの送信バッファへ組み立てる． |
| [`send_escaped_chunk()`](../../../userland/apps/mcpd.c#L388) | 388 | static | バイト列をJSON文字列の内容として送り出す． |
| [`execute_command_stream()`](../../../userland/apps/mcpd.c#L415) | 415 | static | シェルを起動し，最大4 KiBの出力を即時にHTTPへ流す． |
| [`list_tools_stream()`](../../../userland/apps/mcpd.c#L482) | 482 | static | 応答の総サイズをRAMに載せずMCPツール一覧を送信する． |
| [`terminal_open_command()`](../../../userland/apps/mcpd.c#L506) | 506 | static | セッションをHTTP要求から独立して保持し，親の標準FDは必ず元へ戻す． |
| [`terminal_close_session()`](../../../userland/apps/mcpd.c#L541) | 541 | static | 現在のPTYセッションを終了し，Masterと子プロセスを解放する． |
| [`terminal_drain()`](../../../userland/apps/mcpd.c#L551) | 551 | static | Masterを枯渇させないため，screen取得前にANSI出力を消費する． |
| [`terminal_screen_stream()`](../../../userland/apps/mcpd.c#L561) | 561 | static | /dev/pts/0.screenの1,944バイトをchunkedで送る． |
| [`send_full_manual()`](../../../userland/apps/mcpd.c#L602) | 602 | static | ROM内の/manファイルをMCPへストリーミング転送する． |
| [`process_json()`](../../../userland/apps/mcpd.c#L630) | 630 | static | MCPのJSON要求を解析して処理する． |
| [`read_request()`](../../../userland/apps/mcpd.c#L846) | 846 | static | ヘッダ，Content-Length，本文を上限付きで受信する． |
| [`handle()`](../../../userland/apps/mcpd.c#L900) | 900 | static | MCPの接続要求を処理する． |
| [`main()`](../../../userland/apps/mcpd.c#L936) | 936 | 外部 | MCPサーバを起動して接続要求を受け付ける． |

### `userland/apps/mem.c`（17件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`mem_layout()`](../../../userland/apps/mem.c#L21) | 21 | static | カーネル・タスク・共有RAM・ファイル・トレース・ネットワーク領域を表示用に区分する． |
| [`valid()`](../../../userland/apps/mem.c#L36) | 36 | static | 読み出し対象アドレスが公開されたSRAMまたはXIP Flash範囲にあるか確認する． |
| [`hex()`](../../../userland/apps/mem.c#L39) | 39 | static | 指定桁数の16進数を標準出力へ表示する． |
| [`fetch()`](../../../userland/apps/mem.c#L42) | 42 | static | /dev/memにseekして表示対象領域を取得する． |
| [`region()`](../../../userland/apps/mem.c#L45) | 45 | static | 物理アドレスをカーネル，タスク，RAMファイル等の表示領域名に対応付ける． |
| [`symbol_record()`](../../../userland/apps/mem.c#L57) | 57 | static | シンボル表の形式は「アプリ名 TAB 相対16進アドレス TAB 種別 TAB 名前 LF」である． |
| [`scan_symbols()`](../../../userland/apps/mem.c#L71) | 71 | static | シンボル表からアドレスと名前を読み，現在の表示位置と関連付ける． |
| [`task_program()`](../../../userland/apps/mem.c#L143) | 143 | static | /proc/tasksを現在の各RAMスロットの情報源とする． |
| [`refresh_slots()`](../../../userland/apps/mem.c#L187) | 187 | static | /procから生存タスクのスロット情報を取得して表示へ反映する． |
| [`append_app()`](../../../userland/apps/mem.c#L232) | 232 | static | 利用中スロットのプログラム名から表示ラベルを更新する． |
| [`refresh_ram_labels()`](../../../userland/apps/mem.c#L249) | 249 | static | 現在存在するRAMファイルの場所と名前を表示へ反映する． |
| [`refresh_labels()`](../../../userland/apps/mem.c#L273) | 273 | static | メモリアドレスと各領域の名称を再計算する． |
| [`draw()`](../../../userland/apps/mem.c#L320) | 320 | static | アドレス範囲と内容，タスク・シンボルを端末の大きさに合わせて描画する． |
| [`parse()`](../../../userland/apps/mem.c#L357) | 357 | static | 対話操作で入力されたアドレスや数値を解析する． |
| [`shifted()`](../../../userland/apps/mem.c#L360) | 360 | static | 表示するメモリ先頭アドレスを指定量だけ移動する． |
| [`list_symbol_sources()`](../../../userland/apps/mem.c#L367) | 367 | static | /proc/symの対象から選択可能なカーネル／アプリのシンボル表を列挙する． |
| [`main()`](../../../userland/apps/mem.c#L380) | 380 | 外部 | アドレス移動と表示切替を行う対話型メモリ閲覧を開始する． |

### `userland/apps/nc.c`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`port()`](../../../userland/apps/nc.c#L10) | 10 | static | ポート番号の文字列を検査し，16ビットの値へ変換する． |
| [`ipv4()`](../../../userland/apps/nc.c#L18) | 18 | static | lwIPのip4_addr_t.addrはuint32にネットワーク順の値として格納される． |
| [`send_all()`](../../../userland/apps/nc.c#L32) | 32 | static | TCP等の送信可能な範囲へ全データを書き込む． |
| [`usage()`](../../../userland/apps/nc.c#L39) | 39 | static | ncのTCP／UDP接続・待受の使用形式を表示する． |
| [`main()`](../../../userland/apps/nc.c#L45) | 45 | 外部 | TCP・UDPソケットを開き，ネットワークと標準入出力の間でデータを転送する． |

### `userland/apps/net.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`show()`](../../../userland/apps/net.c#L8) | 8 | static | ネットワーク診断用の/procファイルを開き，内容を出力する． |
| [`main()`](../../../userland/apps/net.c#L28) | 28 | 外部 | Wi-Fi・ARP等のネットワーク状態と診断情報を表示する． |

### `userland/apps/omikuji.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`random_from_device()`](../../../userland/apps/omikuji.c#L12) | 12 | static | /dev/randから32ビットの値を読む． |
| [`fortune()`](../../../userland/apps/omikuji.c#L22) | 22 | static | 乱数値をおみくじの結果文字列へ対応付ける． |
| [`main()`](../../../userland/apps/omikuji.c#L36) | 36 | 外部 | 擬似乱数で運勢を選び，結果を表示する． |

### `userland/apps/ping.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`ipv4()`](../../../userland/apps/ping.c#L11) | 11 | static | ドット区切りのIPv4アドレスを4バイトへ変換する． |
| [`main()`](../../../userland/apps/ping.c#L25) | 25 | 外部 | ICMPデバイスを使って指定IPv4宛てにEchoを送信し，応答とタイムアウトを表示する． |

### `userland/apps/pins.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`describe()`](../../../userland/apps/pins.c#L23) | 23 | static | GPIOの割当状態から使用用途の表示文字列を求める． |
| [`pinlabel()`](../../../userland/apps/pins.c#L40) | 40 | static | GPIO番号と現在の使用モードを組み合わせ，基板図に表示するピン名を作る． |
| [`main()`](../../../userland/apps/pins.c#L46) | 46 | 外部 | ボードのGPIO割当と予約状態を一覧表示する． |

### `userland/apps/pong.c`（6件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`sendall()`](../../../userland/apps/pong.c#L16) | 16 | static | 端末FDへの表示データを可能な限り書き切る． |
| [`message()`](../../../userland/apps/pong.c#L28) | 28 | static | 同じ案内文字列を両方の端末FDへ送る． |
| [`row()`](../../../userland/apps/pong.c#L34) | 34 | static | ゲーム盤の指定行に障害物・ラケット・ボールを描画する． |
| [`render()`](../../../userland/apps/pong.c#L41) | 41 | static | ゲーム画面全体を端末へ描画する． |
| [`pong_key()`](../../../userland/apps/pong.c#L79) | 79 | static | 端末ごとにANSI矢印キーを解析する． |
| [`main()`](../../../userland/apps/pong.c#L98) | 98 | 外部 | 端末上でポンゲームを実行し，入力と描画を周期的に処理する． |

### `userland/apps/ps.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/ps.c#L10) | 10 | 外部 | /proc/tasksからPIDと状態などを取得し，プロセス一覧を表示する． |

### `userland/apps/pty_test.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`output_all()`](../../../userland/apps/pty_test.c#L13) | 13 | static | 物理コンソールへの部分書き込みを再試行し，全バイトを転送する． |
| [`main()`](../../../userland/apps/pty_test.c#L28) | 28 | 外部 | 指定されたプログラムをPTYのSlaveで起動し，Masterと実コンソールを中継する． |

### `userland/apps/pwd.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/pwd.c#L7) | 7 | 外部 | 現在の作業ディレクトリを表示する． |

### `userland/apps/pwmctl.c`（7件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`usage()`](../../../userland/apps/pwmctl.c#L10) | 10 | static | PWM周波数・デューティ比の操作方法を表示する． |
| [`eq()`](../../../userland/apps/pwmctl.c#L20) | 20 | static | サブコマンド文字列の一致を確認する． |
| [`read_value()`](../../../userland/apps/pwmctl.c#L23) | 23 | static | PWMデバイスファイルを読み，終端改行を除いた文字列を返す． |
| [`set_value()`](../../../userland/apps/pwmctl.c#L34) | 34 | static | PWMデバイスファイルに数値文字列を書き込み，成功時に0を返す． |
| [`valid_value()`](../../../userland/apps/pwmctl.c#L44) | 44 | static | 周波数またはデューティ比を桁数・範囲・小数形式で検証する． |
| [`status()`](../../../userland/apps/pwmctl.c#L62) | 62 | static | PWMの周波数と各チャネルのデューティ比をデバイスから読み表示する． |
| [`main()`](../../../userland/apps/pwmctl.c#L74) | 74 | 外部 | /dev/pwm/1で周波数・デューティ比の照会と変更を行う． |

### `userland/apps/python.c`（49件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`outn()`](../../../userland/apps/python.c#L16) | 16 | static | 標準出力へ指定長を書き切る． |
| [`outs()`](../../../userland/apps/python.c#L22) | 22 | static | NUL終端文字列を標準出力へ書き込む． |
| [`same()`](../../../userland/apps/python.c#L25) | 25 | static | 二つの識別子やキーワードを文字列として比較する． |
| [`idchar()`](../../../userland/apps/python.c#L28) | 28 | static | 文字が英数字または下線なら真を返す． |
| [`items()`](../../../userland/apps/python.c#L45) | 45 | static | 固定長プール内の指定配列が格納される先頭ポインタを得る． |
| [`string_at()`](../../../userland/apps/python.c#L48) | 48 | static | 固定長プール内の指定文字列が格納される先頭ポインタを得る． |
| [`seqslot()`](../../../userland/apps/python.c#L51) | 51 | static | 変数名に対応する既存の固定スロット番号を探す． |
| [`slot_for()`](../../../userland/apps/python.c#L54) | 54 | static | 変数名のスロットを検索・確保する． |
| [`replace_data()`](../../../userland/apps/python.c#L64) | 64 | static | 配列・文字列を固定長プール内で置き換える． |
| [`seqnew()`](../../../userland/apps/python.c#L91) | 91 | static | 固定長プールに配列・タプル相当の要素列を格納し，変数へ関連付ける． |
| [`setstring()`](../../../userland/apps/python.c#L100) | 100 | static | 最大64文字の文字列を固定長プールに格納する． |
| [`transfer()`](../../../userland/apps/python.c#L112) | 112 | static | 呼出し元が開いたI²C FDで転送する． |
| [`ws()`](../../../userland/apps/python.c#L126) | 126 | static | 解析カーソルを空白・タブの後まで進める． |
| [`eat()`](../../../userland/apps/python.c#L129) | 129 | static | 空白を飛ばした位置で指定1文字を消費する． |
| [`word()`](../../../userland/apps/python.c#L132) | 132 | static | キーワードを単語境界も含めて照合し，成功時だけカーソルを進める． |
| [`getvar()`](../../../userland/apps/python.c#L135) | 135 | static | 名前で整数変数を検索する． |
| [`setvar()`](../../../userland/apps/python.c#L138) | 138 | static | 整数変数へ代入し，旧配列等があればプール領域を解放する． |
| [`divide()`](../../../userland/apps/python.c#L142) | 142 | static | 符号付き32ビットの整数除算を行い，ゼロ除算と最小値の境界を明示的に扱う． |
| [`parse_string()`](../../../userland/apps/python.c#L155) | 155 | static | 文字列リテラルを読み，エスケープと領域上限を検査する． |
| [`payload()`](../../../userland/apps/python.c#L184) | 184 | static | ソケットの送信データは文字列・バイト列・str(整数)から作れる． |
| [`parse_ip()`](../../../userland/apps/python.c#L215) | 215 | static | IPv4のドット区切り表記をncと共通のlwIPアドレス表現へ変換する． |
| [`socket_address()`](../../../userland/apps/python.c#L232) | 232 | static | 通信APIに使うIPv4アドレスとポートを解析する． |
| [`valid_port()`](../../../userland/apps/python.c#L248) | 248 | static | 通信先のポート番号が扱える範囲か検査する． |
| [`parse_fd()`](../../../userland/apps/python.c#L251) | 251 | static | 引数で受け取ったFD番号を検証・変換する． |
| [`read_line()`](../../../userland/apps/python.c#L254) | 254 | static | UART/USBのバイト列から長さ制限のある一行を組み立てる． |
| [`input_text()`](../../../userland/apps/python.c#L271) | 271 | static | input()の文字列を固定領域へ取得する． |
| [`parse_decimal()`](../../../userland/apps/python.c#L292) | 292 | static | 十進文字列を符号付き32ビット整数へ変換する． |
| [`decimal_string()`](../../../userland/apps/python.c#L307) | 307 | static | 32ビット整数を表示用の10進文字列へ変換する． |
| [`integer_conversion()`](../../../userland/apps/python.c#L315) | 315 | static | int()で整数式，文字列変数，input()の結果を整数化する． |
| [`atom()`](../../../userland/apps/python.c#L340) | 340 | static | 整数式の最小単位を解析する． |
| [`product()`](../../../userland/apps/python.c#L487) | 487 | static | 乗除算の優先順位を処理する． |
| [`sum()`](../../../userland/apps/python.c#L493) | 493 | static | 加減算を含む式を評価し，次の構文解析位置を更新する． |
| [`bits()`](../../../userland/apps/python.c#L497) | 497 | static | 再帰下降で演算子の優先順位を扱う． |
| [`comparison()`](../../../userland/apps/python.c#L521) | 521 | static | 比較演算を解析する． |
| [`logical_not()`](../../../userland/apps/python.c#L549) | 549 | static | 比較演算よりnot，さらにand，orの順に優先順位が下がる． |
| [`logical_and()`](../../../userland/apps/python.c#L552) | 552 | static | 論理AND演算を含む式を解析・評価する． |
| [`expr()`](../../../userland/apps/python.c#L560) | 560 | static | 演算子の優先順位に従って式を評価する入口． |
| [`number()`](../../../userland/apps/python.c#L568) | 568 | static | 数値リテラルを解析し，整数値を返す． |
| [`print_value()`](../../../userland/apps/python.c#L573) | 573 | static | 値を出力する． |
| [`parse_i2c_bytes()`](../../../userland/apps/python.c#L613) | 613 | static | I²C送信バイト列を解析する． |
| [`parse_i2c_write_args()`](../../../userland/apps/python.c#L644) | 644 | static | I²C書込みに渡すFD，アドレス，バイト列を解析する． |
| [`statement()`](../../../userland/apps/python.c#L652) | 652 | static | 一文の構文と処理を実行する． |
| [`block_end()`](../../../userland/apps/python.c#L779) | 779 | static | インデントに従って現在の複合文の終端行を探す． |
| [`execute()`](../../../userland/apps/python.c#L783) | 783 | static | インデントで表される文ブロックを実行し，ループ制御を呼出し元へ伝える． |
| [`join_continuations()`](../../../userland/apps/python.c#L858) | 858 | static | リストやbytes等の未閉鎖の括弧に含まれる複数行を，インデントを維持して結合する． |
| [`run_source()`](../../../userland/apps/python.c#L875) | 875 | static | 与えられたソースの先頭から文を順に実行する． |
| [`pending_brackets()`](../../../userland/apps/python.c#L898) | 898 | static | REPL入力に未閉鎖の括弧が残っているか調べる． |
| [`repl()`](../../../userland/apps/python.c#L911) | 911 | static | REPL入力を読み，継続行を処理して一文ずつ実行する． |
| [`main()`](../../../userland/apps/python.c#L952) | 952 | 外部 | ファイル実行と対話実行を引数に応じて切り替える． |

### `userland/apps/reset.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/reset.c#L9) | 9 | 外部 | TTYのカノニカル入力・エコーとANSI表示を復元する． |

### `userland/apps/rm.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/rm.c#L7) | 7 | 外部 | 指定したRAMファイルをVFSのunlinkで削除する． |

### `userland/apps/scroll.c`（9件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`free_space()`](../../../userland/apps/scroll.c#L15) | 15 | static | 文字待ち行列の残容量を返す． |
| [`enqueue()`](../../../userland/apps/scroll.c#L17) | 17 | static | 文字列をリングバッファの末尾に追加する． |
| [`dequeue()`](../../../userland/apps/scroll.c#L21) | 21 | static | リングバッファの先頭から1文字を取り出す． |
| [`valid()`](../../../userland/apps/scroll.c#L23) | 23 | static | 表示対象の印字可能ASCII文字か判定する． |
| [`glyph_bounds()`](../../../userland/apps/scroll.c#L26) | 26 | static | The font has five columns, but most glyphs occupy fewer. |
| [`draw_column()`](../../../userland/apps/scroll.c#L35) | 35 | static | 表示パターンを左へ送って新しい1列を描画する． |
| [`parse_port()`](../../../userland/apps/scroll.c#L41) | 41 | static | UDP待受ポートの十進文字列を検証し数値へ変換する． |
| [`receive_udp()`](../../../userland/apps/scroll.c#L44) | 44 | static | UDP datagrams are plain text. |
| [`main()`](../../../userland/apps/scroll.c#L53) | 53 | 外部 | 標準入力と任意のUDP入力を待ち受け文字をスクロール表示する． |

### `userland/apps/shell.c`（26件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`eq()`](../../../userland/apps/shell.c#L19) | 19 | static | 二つの文字列を完全一致で比較する． |
| [`prefix()`](../../../userland/apps/shell.c#L22) | 22 | static | 指定した接頭辞で文字列が始まるか判定する． |
| [`write_all()`](../../../userland/apps/shell.c#L25) | 25 | static | カーネルのTXキューが満杯なら書込みが途中で終わる． |
| [`put()`](../../../userland/apps/shell.c#L36) | 36 | static | 標準出力へNUL終端文字列を表示する． |
| [`resolve()`](../../../userland/apps/shell.c#L47) | 47 | static | 作業ディレクトリ基準でパスを解決する． |
| [`trim()`](../../../userland/apps/shell.c#L85) | 85 | static | リダイレクト先の名前に空白を含めず，周囲の空白を構文として扱う． |
| [`restore_fd()`](../../../userland/apps/shell.c#L95) | 95 | static | 一時的に変更した標準FDを復元し，退避用FDを閉じる． |
| [`restore_stdio()`](../../../userland/apps/shell.c#L103) | 103 | static | シェルで一時変更した標準入出力FDを元の設定へ戻す． |
| [`redirect()`](../../../userland/apps/shell.c#L109) | 109 | static | ファイルまたはデバイスへ標準FDを付け替える． |
| [`run_external()`](../../../userland/apps/shell.c#L171) | 171 | static | 登録済み外部コマンドをIDで起動する． |
| [`pipeline_spawn()`](../../../userland/apps/shell.c#L225) | 225 | static | パイプの一段を起動し，標準FDの0〜2だけを継承して入出力と前景状態を設定する． |
| [`run_pipeline()`](../../../userland/apps/shell.c#L265) | 265 | static | 最大3段のパイプを組み立てる． |
| [`command()`](../../../userland/apps/shell.c#L315) | 315 | static | 入力行を解釈し，組込みコマンド／外部コマンドへ振り分ける． |
| [`history_add()`](../../../userland/apps/shell.c#L361) | 361 | static | 固定長のコマンド履歴へ追加する． |
| [`shell_move_left()`](../../../userland/apps/shell.c#L373) | 373 | static | 現在位置からn桁だけ左へ移動する． |
| [`shell_columns()`](../../../userland/apps/shell.c#L385) | 385 | static | プロンプトも含め，端末のTAB位置は4桁刻みとする． |
| [`shell_has_tab()`](../../../userland/apps/shell.c#L392) | 392 | static | シェル入力にタブが含まれるか調べ，補完処理の分岐に使う． |
| [`shell_print_expanded()`](../../../userland/apps/shell.c#L398) | 398 | static | 補完候補や展開後の入力行を端末に表示する． |
| [`shell_redraw()`](../../../userland/apps/shell.c#L410) | 410 | static | TABを4桁位置へ展開して一行を再描画する． |
| [`shell_redraw_tail()`](../../../userland/apps/shell.c#L418) | 418 | static | 入力行の変更部分だけ再描画し，プロンプトと前半はそのままにする． |
| [`shell_complete()`](../../../userland/apps/shell.c#L425) | 425 | static | 行末のファイル名または/binコマンドを補完する． |
| [`shell_list_matches()`](../../../userland/apps/shell.c#L495) | 495 | static | 候補を画面に表示して入力行を再描画する． |
| [`shell_readline()`](../../../userland/apps/shell.c#L532) | 532 | static | 一行入力中だけTTYをraw／エコーなしへ切り替え，編集後に元へ戻す． |
| [`run_boot_rc()`](../../../userland/apps/shell.c#L676) | 676 | static | 最初の起動シェルで設定ファイルの各行を通常のコマンド解析器へ渡す． |
| [`run_one_command()`](../../../userland/apps/shell.c#L705) | 705 | static | MCPなどからの一回限りの非対話実行． |
| [`main()`](../../../userland/apps/shell.c#L739) | 739 | 外部 | 端末の用意と起動時処理を終えた後，入力・解析・実行・子の回収を繰り返す． |

### `userland/apps/stty.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`set()`](../../../userland/apps/stty.c#L8) | 8 | static | TTYのカノニカル・エコー等の設定をioctlで変更する． |
| [`main()`](../../../userland/apps/stty.c#L11) | 11 | 外部 | TTY設定を照会・切替する． |

### `userland/apps/tcp_echod.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`port()`](../../../userland/apps/tcp_echod.c#L7) | 7 | static | 待受ポートの数値指定を検査する． |
| [`main()`](../../../userland/apps/tcp_echod.c#L15) | 15 | 外部 | 指定TCPポートで接続を待ち，受信データを送り返す． |

### `userland/apps/tcpdump.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`ip()`](../../../userland/apps/tcpdump.c#L11) | 11 | static | IPv4アドレス4バイトをドット区切りで表示する． |
| [`protocol()`](../../../userland/apps/tcpdump.c#L14) | 14 | static | IPv4プロトコル番号をTCP／UDP／ICMP等の表示名に対応付ける． |
| [`show()`](../../../userland/apps/tcpdump.c#L19) | 19 | static | 保存されたパケット要約を読める形式で出力する． |
| [`main()`](../../../userland/apps/tcpdump.c#L51) | 51 | 外部 | /dev/net/packetから受信・送信パケットの要約を読み，指定件数まで表示する． |

### `userland/apps/trace.c`（29件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`syscall_name()`](../../../userland/apps/trace.c#L13) | 13 | static | トレース中のシステムコール番号を表示名へ変換する． |
| [`load_program_names()`](../../../userland/apps/trace.c#L29) | 29 | static | 登録プログラムのIDと名前をVFSから読み，記録表示に使う． |
| [`program_name()`](../../../userland/apps/trace.c#L54) | 54 | static | プログラムIDから登録名を取得する． |
| [`find_fd()`](../../../userland/apps/trace.c#L63) | 63 | static | 追跡対象プロセスのFDに付けた表示名を探す． |
| [`forget_fd()`](../../../userland/apps/trace.c#L71) | 71 | static | 終了・closeされたFDの表示名キャッシュを削除する． |
| [`save_fd()`](../../../userland/apps/trace.c#L78) | 78 | static | openしたFDと表示パスの組を小さな履歴に保存する． |
| [`col()`](../../../userland/apps/trace.c#L90) | 90 | static | 列幅を固定してトレース表示を小さく予測しやすく保つ． |
| [`numeric()`](../../../userland/apps/trace.c#L99) | 99 | static | 記録内の数値フィールドを表示用に書式化する． |
| [`format_short_time()`](../../../userland/apps/trace.c#L102) | 102 | static | トレース時刻を短い固定桁の表示へ変換する． |
| [`append()`](../../../userland/apps/trace.c#L123) | 123 | static | 文字列バッファ末尾に表示項目を追加し，上限を越えないようにする． |
| [`device_path()`](../../../userland/apps/trace.c#L131) | 131 | static | デバイスIDの名称はカーネルVFSと共通の定義から求める． |
| [`format_target()`](../../../userland/apps/trace.c#L140) | 140 | static | システムコールのFD／対象を人間が読みやすい短い文字列へ整形する． |
| [`print_preview()`](../../../userland/apps/trace.c#L149) | 149 | static | read／writeの先頭データを印字可能な範囲で表示する． |
| [`header()`](../../../userland/apps/trace.c#L169) | 169 | static | トレース表示の項目名と罫線を出力する． |
| [`trace_depth()`](../../../userland/apps/trace.c#L186) | 186 | static | PIDが同じイベントだけが同一の深さを共有する． |
| [`function_label()`](../../../userland/apps/trace.c#L201) | 201 | static | 入口をname，復帰をname/で表す． |
| [`pending_find()`](../../../userland/apps/trace.c#L214) | 214 | static | 未完了システムコールの対象を取得する． |
| [`pending_forget()`](../../../userland/apps/trace.c#L222) | 222 | static | システムコール終了時に対応付けを消し，次回の同一呼び出しへ持ち越さない． |
| [`pending_save()`](../../../userland/apps/trace.c#L229) | 229 | static | 同一PID・同一システムコールの入口は上書きする． |
| [`reset_view_state()`](../../../userland/apps/trace.c#L241) | 241 | static | 対象PIDやFD履歴等のトレース表示状態を初期化する． |
| [`print_pid_separator()`](../../../userland/apps/trace.c#L256) | 256 | static | 表示上のPIDが変わる直前に，ASCIIの区切り線を1行出す． |
| [`print_event()`](../../../userland/apps/trace.c#L264) | 264 | static | 表示は Function（入口name／復帰name/の階層），Arg，Ret，Info に統一する． |
| [`compact_pair()`](../../../userland/apps/trace.c#L378) | 378 | static | ミリ秒表示が同一で，連続した同じ関数・実行コンテキストの入退場だけを統合する． |
| [`display_matches()`](../../../userland/apps/trace.c#L387) | 387 | static | PIDフィルタは表示のみに適用する． |
| [`display_event()`](../../../userland/apps/trace.c#L393) | 393 | static | トレースイベントを表示し，関数の入口と出口を可能なら統合する． |
| [`display_flush()`](../../../userland/apps/trace.c#L421) | 421 | static | 表示待ちの関数入口イベントを出力する． |
| [`trace_drain()`](../../../userland/apps/trace.c#L432) | 432 | static | トレースデバイスから固定長レコードを読み，表示へ順次流す． |
| [`trace_run()`](../../../userland/apps/trace.c#L440) | 440 | static | コマンドの実行中だけ記録範囲を切り替え，既存の記録状態を復元する． |
| [`main()`](../../../userland/apps/trace.c#L529) | 529 | 外部 | トレース表示・起動操作をコマンド引数に基づいて選ぶ． |

### `userland/apps/tree.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`traverse()`](../../../userland/apps/tree.c#L10) | 10 | static | 指定パスを階層的に探索し，深さ上限を守って子をたどる． |
| [`visit()`](../../../userland/apps/tree.c#L18) | 18 | static | ディレクトリエントリを木構造の字下げ付きで表示する． |
| [`main()`](../../../userland/apps/tree.c#L27) | 27 | 外部 | VFSディレクトリを再帰的にたどり，階層をツリー表示する． |

### `userland/apps/tty.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../userland/apps/tty.c#L7) | 7 | 外部 | 現在タスクが接続されているTTYの情報を表示する． |

### `userland/apps/udp_echod.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`port()`](../../../userland/apps/udp_echod.c#L7) | 7 | static | UDP待受ポート番号を解析する． |
| [`main()`](../../../userland/apps/udp_echod.c#L15) | 15 | 外部 | UDP受信データを送信元へ送り返すエコーサーバを動かす． |

### `userland/apps/vi.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`poll_key()`](../../../userland/apps/vi.c#L12) | 12 | static | ANSIカーソルキーの残りを読み出し，単独Escをブロックしない． |
| [`move_arrow()`](../../../userland/apps/vi.c#L26) | 26 | static | エスケープシーケンスによるカーソル移動指示を処理する． |
| [`main()`](../../../userland/apps/vi.c#L35) | 35 | 外部 | 共通の編集処理を用い，vi風キー操作でRAMファイルを編集する． |

### `userland/apps/walk.h`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`walk_eq()`](../../../userland/apps/walk.h#L14) | 14 | static | 二つのパス構成要素を一致比較する． |
| [`walk_normalize()`](../../../userland/apps/walk.h#L17) | 17 | static | 作業ディレクトリと相対パスを解決し，.と..を含む経路を正規化する． |
| [`walk_dir()`](../../../userland/apps/walk.h#L44) | 44 | static | 指定したパスがgetdentsで列挙できるディレクトリか判定する． |
| [`walk_entries()`](../../../userland/apps/walk.h#L51) | 51 | static | ディレクトリを列挙してコールバックへ名前と種別を渡す． |
| [`walk_join()`](../../../userland/apps/walk.h#L62) | 62 | static | 親ディレクトリと子の名前を結合する． |

### `userland/apps/watch.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`capture_byte()`](../../../userland/apps/watch.c#L23) | 23 | static | 子プロセスの出力1バイトを画面スナップショットへ反映する． |
| [`redraw()`](../../../userland/apps/watch.c#L97) | 97 | static | 対象コマンドの出力を再描画し，画面を更新する． |
| [`capture_command()`](../../../userland/apps/watch.c#L109) | 109 | static | 256バイトのカーネルパイプは子が動いている間に排出する． |
| [`main()`](../../../userland/apps/watch.c#L141) | 141 | 外部 | 指定コマンドを一定間隔で再実行し，結果を端末へ表示する． |

## ライブラリ

### `userland/lib/libc.c`（59件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`strlen()`](../../../userland/lib/libc.c#L28) | 28 | 外部 | NUL終端文字列の長さを数える． |
| [`strcpy()`](../../../userland/lib/libc.c#L31) | 31 | 外部 | 文字列を終端文字まで宛先へコピーし，宛先を返す． |
| [`strncpy()`](../../../userland/lib/libc.c#L34) | 34 | 外部 | 最大nバイトをコピーする． |
| [`strcat()`](../../../userland/lib/libc.c#L37) | 37 | 外部 | 宛先文字列の末尾に別の文字列を追加する． |
| [`strcmp()`](../../../userland/lib/libc.c#L40) | 40 | 外部 | 二つの文字列を辞書順で比較し，差の符号を返す． |
| [`strncmp()`](../../../userland/lib/libc.c#L43) | 43 | 外部 | 文字列を最大n文字比較し，最初に異なるバイトの差を返す． |
| [`strchr()`](../../../userland/lib/libc.c#L46) | 46 | 外部 | 文字列から指定文字を探し，最初の位置またはNULLを返す． |
| [`strstr()`](../../../userland/lib/libc.c#L49) | 49 | 外部 | 文字列から部分文字列を探し，最初の位置またはNULLを返す． |
| [`memcpy()`](../../../userland/lib/libc.c#L52) | 52 | 外部 | バイト単位の処理にして，コンパイラが外部libc呼出しを生成しないようにする． |
| [`memmove()`](../../../userland/lib/libc.c#L55) | 55 | 外部 | 領域が重なる場合も正しい順序でnバイト移動する． |
| [`memset()`](../../../userland/lib/libc.c#L58) | 58 | 外部 | 指定領域のnバイトを同じ値で埋める． |
| [`__aeabi_memcpy()`](../../../userland/lib/libc.c#L61) | 61 | 外部 | Clang -Ozは構造体コピーなどをARM EABIのメモリ操作へ変換することがある． |
| [`__aeabi_memcpy4()`](../../../userland/lib/libc.c#L64) | 64 | 外部 | ARM EABIの4バイト整列コピー要求を自前のmemcpyへ委譲する． |
| [`__aeabi_memcpy8()`](../../../userland/lib/libc.c#L67) | 67 | 外部 | ARM EABIの8バイト整列コピー要求を自前のmemcpyへ委譲する． |
| [`__aeabi_memmove()`](../../../userland/lib/libc.c#L70) | 70 | 外部 | ARM EABIの重複可能なメモリ移動を自前のmemmoveへ委譲する． |
| [`__aeabi_memset()`](../../../userland/lib/libc.c#L73) | 73 | 外部 | EABIのmemsetはISO Cのmemsetと異なり，引数順が宛先・長さ・値である． |
| [`__aeabi_memset4()`](../../../userland/lib/libc.c#L76) | 76 | 外部 | ARM EABIの4バイト整列初期化を自前のmemsetへ委譲する． |
| [`__aeabi_memset8()`](../../../userland/lib/libc.c#L79) | 79 | 外部 | ARM EABIの8バイト整列初期化を自前のmemsetへ委譲する． |
| [`memchr()`](../../../userland/lib/libc.c#L82) | 82 | 外部 | 最大nバイトを検索し，最初に指定値と一致した位置を返す． |
| [`memcmp()`](../../../userland/lib/libc.c#L89) | 89 | 外部 | 指定長の二つのメモリ領域を比較し，最初の不一致の差を返す． |
| [`abs()`](../../../userland/lib/libc.c#L92) | 92 | 外部 | 整数の絶対値を返す． |
| [`strtol()`](../../../userland/lib/libc.c#L94) | 94 | 外部 | ISO Cと同様にabs(INT_MIN)の動作は未定義である． |
| [`strtoul()`](../../../userland/lib/libc.c#L119) | 119 | 外部 | 符号と基数接頭辞を解釈して整数へ変換する． |
| [`atoi()`](../../../userland/lib/libc.c#L149) | 149 | 外部 | 文字列の先頭を符号付き10進整数として読み取る． |
| [`output()`](../../../userland/lib/libc.c#L152) | 152 | static | 標準出力のバッファを部分書込みまで処理して送る． |
| [`fflush()`](../../../userland/lib/libc.c#L165) | 165 | 外部 | 保留中の文字を送信する． |
| [`setvbuf()`](../../../userland/lib/libc.c#L188) | 188 | 外部 | FILEストリームのバッファリング方式を設定する． |
| [`stream_put()`](../../../userland/lib/libc.c#L199) | 199 | static | FILEの出力条件を検査して1文字送る． |
| [`stream_get()`](../../../userland/lib/libc.c#L209) | 209 | static | FILEの読出し位置とungetc用保存バイトを管理する． |
| [`stream_unget()`](../../../userland/lib/libc.c#L223) | 223 | static | 直前に読んだ1文字をFILEへ戻し，次回読出しへ備える． |
| [`feof()`](../../../userland/lib/libc.c#L230) | 230 | 外部 | FILEストリームの終端到達フラグを返す． |
| [`ferror()`](../../../userland/lib/libc.c#L233) | 233 | 外部 | FILEストリームの入出力エラーフラグを返す． |
| [`fopen()`](../../../userland/lib/libc.c#L236) | 236 | 外部 | openしたFDを小さなFILE管理表に関連付ける． |
| [`fclose()`](../../../userland/lib/libc.c#L254) | 254 | 外部 | 保留中の出力を流し，FDとFILE管理枠を解放する． |
| [`fread()`](../../../userland/lib/libc.c#L262) | 262 | 外部 | 指定サイズ・個数で入力を読み取る． |
| [`fwrite()`](../../../userland/lib/libc.c#L271) | 271 | 外部 | 指定サイズ・個数の出力を行う． |
| [`putchar()`](../../../userland/lib/libc.c#L280) | 280 | 外部 | stdoutへ1文字出力する． |
| [`getchar()`](../../../userland/lib/libc.c#L283) | 283 | 外部 | stdinから1文字取得する． |
| [`puts()`](../../../userland/lib/libc.c#L286) | 286 | 外部 | 文字列をstdoutへ出力し，末尾に改行を付ける． |
| [`fgets()`](../../../userland/lib/libc.c#L292) | 292 | 外部 | FILEから最大size-1文字を読み，改行またはEOFまでをNUL終端で格納する． |
| [`emit()`](../../../userland/lib/libc.c#L302) | 302 | static | 書式出力の1文字を，ファイルまたは文字列バッファの出力先へ渡す． |
| [`digits()`](../../../userland/lib/libc.c#L309) | 309 | static | 指定基数で整数を桁の文字列へ変換する． |
| [`pad()`](../../../userland/lib/libc.c#L316) | 316 | static | 書式指定の最小幅に合わせて空白等を出力する． |
| [`format()`](../../../userland/lib/libc.c#L319) | 319 | static | 限定的なprintf書式指定を解釈して文字を出力する． |
| [`printf()`](../../../userland/lib/libc.c#L371) | 371 | 外部 | 簡易書式解析でstdoutへ整形出力する． |
| [`fprintf()`](../../../userland/lib/libc.c#L374) | 374 | 外部 | 簡易書式解析で指定FILEへ整形出力する． |
| [`snprintf()`](../../../userland/lib/libc.c#L377) | 377 | 外部 | 最大サイズを守って整形文字列を生成する． |
| [`sprintf()`](../../../userland/lib/libc.c#L380) | 380 | 外部 | サイズ指定のない書式出力を行う． |
| [`scan_get()`](../../../userland/lib/libc.c#L385) | 385 | static | 書式入力処理で次の1文字を取り出す． |
| [`scan_unget()`](../../../userland/lib/libc.c#L398) | 398 | static | 書式入力処理で読んだ1文字を入力元へ戻す． |
| [`scan_space()`](../../../userland/lib/libc.c#L406) | 406 | static | 書式入力で先頭の空白を読み飛ばす． |
| [`scan_digit()`](../../../userland/lib/libc.c#L409) | 409 | static | 書式入力で1桁の数字を指定基数として解釈する． |
| [`scan_run()`](../../../userland/lib/libc.c#L417) | 417 | static | 限定的なscanf書式を解析する． |
| [`scanf()`](../../../userland/lib/libc.c#L513) | 513 | 外部 | stdinから簡易書式に従って値を読み取る． |
| [`fscanf()`](../../../userland/lib/libc.c#L519) | 519 | 外部 | 指定FILEから簡易書式に従って値を読み取る． |
| [`sscanf()`](../../../userland/lib/libc.c#L526) | 526 | 外部 | 入力文字列から簡易書式に従って値を読み取る． |
| [`__aeabi_memclr()`](../../../userland/lib/libc.c#L532) | 532 | 外部 | 一部のコンパイラがローカル構造体のゼロ初期化で生成するARM EABI補助関数． |
| [`__aeabi_memclr4()`](../../../userland/lib/libc.c#L539) | 539 | 外部 | ARM EABIの4バイト整列ゼロクリア要求をmemsetへ委譲する． |
| [`__aeabi_memclr8()`](../../../userland/lib/libc.c#L542) | 542 | 外部 | ARM EABIの8バイト整列ゼロクリア要求をmemsetへ委譲する． |

### `userland/lib/screen.h`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`screen_enter()`](../../../userland/lib/screen.h#L13) | 13 | static | 全画面操作用に代替画面へ切り替え，カーソルを隠す． |
| [`screen_leave()`](../../../userland/lib/screen.h#L16) | 16 | static | 全画面表示を終え，元の端末画面とカーソルを復元する． |
| [`screen_move()`](../../../userland/lib/screen.h#L19) | 19 | static | ANSIエスケープシーケンスで指定行・列へカーソルを移す． |
| [`screen_measure()`](../../../userland/lib/screen.h#L22) | 22 | static | 端末サイズを問い合わせ，取得できなければ指定された既定値を利用する． |

### `userland/lib/syscalls.c`（50件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`call()`](../../../userland/lib/syscalls.c#L8) | 8 | static | r0にシステムコール番号n，r1〜r3に引数a〜cを置いてSVCを発行する． |
| [`chdir()`](../../../userland/lib/syscalls.c#L18) | 18 | 外部 | SVCを発行して現在の作業ディレクトリを変更する． |
| [`getcwd()`](../../../userland/lib/syscalls.c#L21) | 21 | 外部 | SVCから現在ディレクトリのパスを受け取る． |
| [`getpid()`](../../../userland/lib/syscalls.c#L24) | 24 | 外部 | SVCから現在タスクのPIDを取得する． |
| [`flash_id()`](../../../userland/lib/syscalls.c#L27) | 27 | 外部 | SVCからFlash固有IDの下位32ビットを取得する． |
| [`kill()`](../../../userland/lib/syscalls.c#L30) | 30 | 外部 | SVCで指定PIDのタスクに終了を要求する． |
| [`_exit()`](../../../userland/lib/syscalls.c#L33) | 33 | 外部 | SVCで現在タスクを終了する． |
| [`msleep()`](../../../userland/lib/syscalls.c#L36) | 36 | 外部 | SVCで指定ミリ秒だけタスクを睡眠させる． |
| [`yield()`](../../../userland/lib/syscalls.c#L39) | 39 | 外部 | SVCで実行権をほかのREADYタスクへ譲る． |
| [`open()`](../../../userland/lib/syscalls.c#L42) | 42 | 外部 | SVCでVFS上のパスを開き，FDまたはエラーを返す． |
| [`close()`](../../../userland/lib/syscalls.c#L45) | 45 | 外部 | SVCでFDを閉じる． |
| [`lseek()`](../../../userland/lib/syscalls.c#L48) | 48 | 外部 | SVCでFDのファイル位置を変更する． |
| [`select()`](../../../userland/lib/syscalls.c#L51) | 51 | 外部 | FDマスクと待機時間を一つの要求にまとめてSVCへ渡す． |
| [`read()`](../../../userland/lib/syscalls.c#L73) | 73 | 外部 | SVCで指定FDから最大nバイトを読み込む． |
| [`write()`](../../../userland/lib/syscalls.c#L76) | 76 | 外部 | SVCで指定FDへ最大nバイトを書き込む． |
| [`ioctl()`](../../../userland/lib/syscalls.c#L79) | 79 | 外部 | SVCでTTYやデバイスの固有制御を要求する． |
| [`fcntl()`](../../../userland/lib/syscalls.c#L82) | 82 | 外部 | SVCでFDのフラグ・非ブロック設定等を操作する． |
| [`waitpid()`](../../../userland/lib/syscalls.c#L85) | 85 | 外部 | SVCで指定タスクの終了を待ち，必要なら終了状態を受け取る． |
| [`getdents()`](../../../userland/lib/syscalls.c#L88) | 88 | 外部 | SVCでディレクトリエントリを一件ずつ読み取る． |
| [`unlink()`](../../../userland/lib/syscalls.c#L91) | 91 | 外部 | SVCでVFS上のRAMファイルを削除する． |
| [`pipe()`](../../../userland/lib/syscalls.c#L94) | 94 | 外部 | SVCでパイプの読取FD・書込FDを配列へ返す． |
| [`dup()`](../../../userland/lib/syscalls.c#L97) | 97 | 外部 | SVCで既存FDを未使用番号へ複製する． |
| [`dup2()`](../../../userland/lib/syscalls.c#L100) | 100 | 外部 | SVCで既存FDを指定FD番号へ複製する． |
| [`program_id()`](../../../userland/lib/syscalls.c#L103) | 103 | 外部 | 名前に対応する登録済み実行プログラムのIDを取得する． |
| [`spawn()`](../../../userland/lib/syscalls.c#L106) | 106 | 外部 | プログラムIDと引数で子タスクを起動し，PIDを返す． |
| [`spawn_foreground()`](../../../userland/lib/syscalls.c#L109) | 109 | 外部 | 子タスクを前景指定付きで起動する． |
| [`tty_spawn()`](../../../userland/lib/syscalls.c#L112) | 112 | 外部 | 指定TTY向けに登録プログラムを起動する． |
| [`set_foreground()`](../../../userland/lib/syscalls.c#L115) | 115 | 外部 | TTYの前景プロセスを指定PIDへ切り替える． |
| [`console_ready()`](../../../userland/lib/syscalls.c#L118) | 118 | 外部 | コンソールが利用可能な状態かSVCで問い合わせる． |
| [`get_tty()`](../../../userland/lib/syscalls.c#L121) | 121 | 外部 | 現在のタスクが所属するTTY番号を取得する． |
| [`spawn_tmp()`](../../../userland/lib/syscalls.c#L124) | 124 | 外部 | /tmpの実行イメージを読み出し，子タスクとして起動する． |
| [`spawn_tmp_foreground()`](../../../userland/lib/syscalls.c#L127) | 127 | 外部 | /tmpの実行イメージをTTY前景タスクとして起動する． |
| [`socket()`](../../../userland/lib/syscalls.c#L130) | 130 | 外部 | ドメイン・種別・プロトコルを検査するカーネルへソケット作成を要求する． |
| [`bind_udp()`](../../../userland/lib/syscalls.c#L133) | 133 | 外部 | UDPソケットを指定ポートへ束縛する． |
| [`sendto_udp()`](../../../userland/lib/syscalls.c#L136) | 136 | 外部 | 宛先情報を含むPicoNIX独自UDP構造体で送信する． |
| [`recvfrom_udp()`](../../../userland/lib/syscalls.c#L139) | 139 | 外部 | PicoNIX独自UDP構造体へ送信元と受信内容を取得する． |
| [`tcp_listen()`](../../../userland/lib/syscalls.c#L142) | 142 | 外部 | システムコール番号はkernel/kernel.cと共通の定義を使う． |
| [`tcp_connect()`](../../../userland/lib/syscalls.c#L145) | 145 | 外部 | 接続先構造体を渡し，TCP接続を非同期に開始する． |
| [`tcp_status()`](../../../userland/lib/syscalls.c#L148) | 148 | 外部 | TCPソケットの接続状態をカーネルへ問い合わせる． |
| [`tcp_accept()`](../../../userland/lib/syscalls.c#L151) | 151 | 外部 | TCP待受FDから確立済み接続を取り出す． |
| [`ipv4_addr()`](../../../userland/lib/syscalls.c#L154) | 154 | static | BSD風ソケットのアドレスをIPv4に限定して検査する． |
| [`bind()`](../../../userland/lib/syscalls.c#L163) | 163 | 外部 | bindのIPv4アドレスとポートを検証し，PicoNIXのUDP bindまたはTCP listenの呼出しに変換する． |
| [`listen()`](../../../userland/lib/syscalls.c#L179) | 179 | 外部 | PicoNIXではTCPのbind時点で待受を開始するため，ここでは引数の簡易検証のみ行う． |
| [`accept()`](../../../userland/lib/syscalls.c#L185) | 185 | 外部 | 未対応の接続元アドレス取得を拒否して，TCP接続FDを受け取る． |
| [`connect()`](../../../userland/lib/syscalls.c#L191) | 191 | 外部 | IPv4のsockaddr_inをPicoNIXのエンドポイントへ変換してTCP接続する． |
| [`shutdown()`](../../../userland/lib/syscalls.c#L201) | 201 | 外部 | 送信側だけの半閉鎖（SHUT_WR）をサポートする． |
| [`send()`](../../../userland/lib/syscalls.c#L208) | 208 | 外部 | 送信フラグとバッファを検証し，writeへ委譲する． |
| [`recv()`](../../../userland/lib/syscalls.c#L214) | 214 | 外部 | 受信フラグとバッファを検証し，readへ委譲する． |
| [`sendto()`](../../../userland/lib/syscalls.c#L220) | 220 | 外部 | IPv4宛先を独自UDP送信構造体へ変換し，データを送信する． |
| [`recvfrom()`](../../../userland/lib/syscalls.c#L234) | 234 | 外部 | UDPデータを受信し，必要なら送信元をsockaddr_inへ戻す． |

### `userland/lib/unistd.h`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`htons()`](../../../userland/lib/unistd.h#L59) | 59 | static | 16ビット整数のバイト順を入れ替える． |

## ホスト用ユーティリティ

### `host/demo_udp.py`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`run()`](../../../host/demo_udp.py#L17) | 17 | モジュール | UDPデバイス制御サービスへ要求を送信し，結果を表示する実演処理． |
| [`main()`](../../../host/demo_udp.py#L43) | 43 | モジュール | UDP 2000番のGPIO等の遠隔操作を試すコマンド引数を処理する． |

### `host/mcp_client.py`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`parse_args()`](../../../host/mcp_client.py#L13) | 13 | モジュール | コマンドライン引数を解析する． |
| [`run()`](../../../host/mcp_client.py#L32) | 32 | モジュール | MCP接続を確立して要求を送る． |
| [`main()`](../../../host/mcp_client.py#L73) | 73 | モジュール | 非同期クライアント処理を起動する． |

### `host/mcp_llm_client.py`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`styled()`](../../../host/mcp_llm_client.py#L37) | 37 | モジュール | Colorize labels only; never modify message content sent to the LLM. |
| [`show()`](../../../host/mcp_llm_client.py#L44) | 44 | モジュール | Show a labeled message, coloring tool and status lines when supported. |
| [`reasoning_from_delta()`](../../../host/mcp_llm_client.py#L52) | 52 | モジュール | Ollama can expose thinking as reasoning_content or reasoning. |
| [`stream_reply()`](../../../host/mcp_llm_client.py#L63) | 63 | モジュール | Print streamed reasoning/content and reassemble fragmented tool calls. |
| [`main()`](../../../host/mcp_llm_client.py#L149) | 149 | モジュール | MCPクライアントとLLMの対話を実行する． |

### `host/mcp_pty_test.py`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../../../host/mcp_pty_test.py#L7) | 7 | モジュール | MCP SDKでPTYを起動し，画面取得とキー入力を実機検証する． |

### `host/mcp_stress.py`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`send_request()`](../../../host/mcp_stress.py#L14) | 14 | モジュール | One TCP transaction; never retry an ambiguous tool execution. |
| [`main()`](../../../host/mcp_stress.py#L58) | 58 | モジュール | Run a bounded sequence of distinct HTTP transactions and summarize failures. |

### `host/piconixctl.py`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`request()`](../../../host/piconixctl.py#L9) | 9 | モジュール | PicoNIXのUDPデバイス制御サービスへコマンドを送り，応答を受け取る． |
| [`main()`](../../../host/piconixctl.py#L22) | 22 | モジュール | 接続先・操作対象を解析し，PicoNIX向けのデバイス制御要求を実行する． |
