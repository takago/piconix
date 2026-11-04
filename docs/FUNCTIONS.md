# 関数一覧（自動生成）

この文書は `python3 tools/gen_functions.py` でソースから自動生成します．
**手で編集せず，関数直前の日本語コメントを修正して再生成してください．**

- 対象：`kernel/`，`include/`，`userland/`，`host/`，`tools/` のC関数定義，Pythonトップレベル関数，アセンブリの関数ラベル．
- ヘッダは**関数定義のみ**を掲載します（関数宣言・マクロ・型定義は対象外）．
- `tools/test_scripts/` の回帰テストは対象外です．関数定義がないファイルも省略します．
- 行番号はこの文書を生成した時点の行番号です．変更後は再生成してください．
- 概要の `—` は関数直前に説明コメントがないことを表します．関数名からの推測は行いません．
- 範囲欄の `static` はCの内部結合，`外部` は外部結合，`モジュール` と `内部名` はPythonの名前付け，`大域` はアセンブリラベルを表します．外部結合は公開APIを意味しません．
- C言語の解析は簡易的な構文走査で，プリプロセッサの条件分岐を評価しません．環境依存の定義は両方載る場合があります．

**掲載関数：729件（概要コメントあり：729件）**

## ファイル一覧

- [カーネル](#カーネル)：14ファイル
- [ユーザプログラムとライブラリ](#ユーザプログラムとライブラリ)：54ファイル
- [開発・ホストツール](#開発・ホストツール)：10ファイル

## カーネル

### `kernel/adc.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`adc_driver_init()`](../kernel/adc.c#L10) | 10 | 外部 | ADCと温度センサを初期化し，利用可能な入力ピンをADC機能に切り替える． |
| [`adc_driver_read()`](../kernel/adc.c#L17) | 17 | 外部 | 指定ADCチャネルを選択して生の12ビット測定値を返す． |
| [`adc_driver_format()`](../kernel/adc.c#L23) | 23 | 外部 | ADC値をテキストへ変換する． |

### `kernel/context.S`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`isr_pendsv()`](../kernel/context.S#L12) | 12 | 大域 | PendSV例外でユーザタスクのレジスタとPSPを退避し，スケジューラが選んだタスクを復元する． |
| [`isr_svcall()`](../kernel/context.S#L54) | 54 | 大域 | SVC例外で現在のスタックフレームを求め，Cのシステムコールディスパッチャへ渡す． |

### `kernel/fb.c`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`fb_init()`](../kernel/fb.c#L17) | 17 | 外部 | 8×8 LEDマトリクスの表示状態と走査位置を初期化する． |
| [`fb_active()`](../kernel/fb.c#L20) | 20 | 外部 | フレームバッファ表示が有効なら真を返す． |
| [`fb_tick()`](../kernel/fb.c#L23) | 23 | 外部 | タイマ割込みごとに表示行を切り替える． |
| [`fb_ioctl()`](../kernel/fb.c#L35) | 35 | 外部 | 表示の有効化・無効化・状態取得を処理する． |
| [`fb_rw()`](../kernel/fb.c#L62) | 62 | 外部 | 8バイトの表示ビット列を読み書きする． |

### `kernel/gpio.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`gpio_driver_init()`](../kernel/gpio.c#L10) | 10 | 外部 | ボード上のLEDとスイッチを含むGPIOの初期設定を行う． |
| [`gpio_driver_rw()`](../kernel/gpio.c#L24) | 24 | 外部 | GPIO値・方向等をデバイスファイル経由で読み書きする． |
| [`gpio_driver_ioctl()`](../kernel/gpio.c#L67) | 67 | 外部 | GPIOの入力・出力モード等を変更する． |

### `kernel/i2c.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`i2c_driver_init()`](../kernel/i2c.c#L9) | 9 | 外部 | I2C1を初期化し，使用ピンをI2C機能へ設定する． |
| [`i2c_driver_transfer()`](../kernel/i2c.c#L18) | 18 | 外部 | アドレス・転送長を検査し，I2Cの書込みと読出しを一つの要求として実行する． |

### `kernel/kernel.c`（128件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`piconix_flash_id()`](../kernel/kernel.c#L30) | 30 | 外部 | Flash固有IDの下位32ビットを組み立てる． |
| [`vfs_fixed_path_id()`](../kernel/kernel.c#L107) | 107 | static | 固定デバイス／仮想ファイルの絶対パスを内部IDに変換する． |
| [`vfs_directory_id()`](../kernel/kernel.c#L121) | 121 | static | 固定ディレクトリ名をIDへ変換する． |
| [`vfs_directory_path()`](../kernel/kernel.c#L128) | 128 | static | 固定ディレクトリIDから絶対パスを求める． |
| [`vfs_fixed_id_path()`](../kernel/kernel.c#L133) | 133 | static | 登録済みのデバイス・仮想ファイルIDをパスに戻す． |
| [`vfs_device_count()`](../kernel/kernel.c#L142) | 142 | static | 固定デバイス登録表の項目数を返す． |
| [`vfs_device_path()`](../kernel/kernel.c#L145) | 145 | static | 固定デバイス表の指定番号に対応するパスを返す． |
| [`vfs_gpio_id()`](../kernel/kernel.c#L150) | 150 | static | GPIOのファイル名を内部IDへ変換する． |
| [`pipe_end()`](../kernel/kernel.c#L179) | 179 | static | デバイスIDがパイプの読取端・書込端に属するか判定する． |
| [`pipe_for_dev()`](../kernel/kernel.c#L182) | 182 | static | パイプ端点のデバイスIDから，対応する固定長パイプ管理領域を求める． |
| [`pipe_is_writer()`](../kernel/kernel.c#L185) | 185 | static | パイプ端点のIDから書込側かどうかを判定する． |
| [`pipe_ref()`](../kernel/kernel.c#L188) | 188 | static | パイプ端点の参照数をdeltaだけ増減する． |
| [`pipe_transfer()`](../kernel/kernel.c#L197) | 197 | static | 固定256バイトのリングで部分読み書きを行う． |
| [`rand32()`](../kernel/kernel.c#L238) | 238 | static | xorshiftによる32ビット擬似乱数を更新して返す． |
| [`trace_record()`](../kernel/kernel.c#L292) | 292 | static | 32バイトのトレースイベントを16 KiBリングへ記録する． |
| [`foreground_child()`](../kernel/kernel.c#L331) | 331 | static | 指定PIDの子を現在のTTYの前景へ移す． |
| [`tty_owner_pid()`](../kernel/kernel.c#L356) | 356 | static | TTYを占有するプロセスを求める． |
| [`take_exit()`](../kernel/kernel.c#L361) | 361 | static | 親PIDに紐づく終了通知を一件取り出す． |
| [`fd_set()`](../kernel/kernel.c#L373) | 373 | static | 一つのFDに関連する内部ID，位置，フラグ，対象PIDをまとめて設定する． |
| [`tcp_fd_slot()`](../kernel/kernel.c#L384) | 384 | static | タスク内のTCP用FDを検証し，対応するネットワーク管理スロットを返す． |
| [`tcp_other_fd()`](../kernel/kernel.c#L392) | 392 | static | 同一TCP管理スロットを参照する別のFDを探し，重複参照の後始末に使う． |
| [`fd_is_socket()`](../kernel/kernel.c#L400) | 400 | static | ソケットFDは一つのlwIP資源を所有するため，パイプと異なりdup()の対象外とする． |
| [`fd_release()`](../kernel/kernel.c#L406) | 406 | static | FDが保持する資源を解放する． |
| [`fd_inherit()`](../kernel/kernel.c#L429) | 429 | static | FDを子／別番号へ複製する． |
| [`fd_copy()`](../kernel/kernel.c#L437) | 437 | static | 同一タスク内でFDの参照先と状態を複製する． |
| [`task_active()`](../kernel/kernel.c#L442) | 442 | static | 終了済みのタスクスロットは直ちに再利用できる． |
| [`tty_foreground_survivor()`](../kernel/kernel.c#L447) | 447 | static | パイプ段のうち現在生存する前景プロセスを探す． |
| [`finish_task()`](../kernel/kernel.c#L459) | 459 | static | タスク終了時に通知，FD，通信資源，TTYの前景・モードを後始末する． |
| [`task_unblock()`](../kernel/kernel.c#L513) | 513 | static | 中断中のSVCの保存フレームへ戻り値を書き込み，待機情報を消してREADYにする． |
| [`wake_pipes()`](../kernel/kernel.c#L526) | 526 | static | ブロック中のパイプread/writeを再試行する． |
| [`wake_waiters()`](../kernel/kernel.c#L545) | 545 | static | 子の終了を待っていた親を起こす． |
| [`free_program_slot()`](../kernel/kernel.c#L562) | 562 | static | 未使用または終了済みの24 KiBユーザスロットを探す． |
| [`os_register_ram_image()`](../kernel/kernel.c#L571) | 571 | 外部 | Flash上のPICイメージとゼロ初期化範囲を登録する． |
| [`tty_mode_reset()`](../kernel/kernel.c#L586) | 586 | static | TTYを標準の行入力・エコー・改行変換へ戻し，未完了入力も初期化する． |
| [`tty_raw_enqueue()`](../kernel/kernel.c#L594) | 594 | static | 非カノニカル入力を小さな固定キューへ積む． |
| [`tty_enqueue()`](../kernel/kernel.c#L613) | 613 | static | 編集済み行を受信キューへ移す． |
| [`tty_dequeue()`](../kernel/kernel.c#L622) | 622 | static | TTYキューから最大nバイト返す． |
| [`uart_pop()`](../kernel/kernel.c#L637) | 637 | static | 指定TTYの受信リングから1バイト取り出す． |
| [`uart1_read()`](../kernel/kernel.c#L645) | 645 | static | UART1をTTY所有状態に応じて読み分ける． |
| [`uart_push()`](../kernel/kernel.c#L656) | 656 | static | 受信リングへ1バイト追加する． |
| [`console_queue()`](../kernel/kernel.c#L665) | 665 | static | USB CDCへの送信データを固定長リングへ積む． |
| [`console_tx_poll()`](../kernel/kernel.c#L677) | 677 | static | アイドルタスク側でUSB送信リングを少量ずつ空にする． |
| [`tty_put()`](../kernel/kernel.c#L693) | 693 | static | USB CDCコンソールの送信キューへ1文字を渡す． |
| [`tty_put_dev()`](../kernel/kernel.c#L698) | 698 | static | TTY番号に応じてUSB CDC，UART1，Wi-Fi端末へ1文字出力する． |
| [`tty_tx_char()`](../kernel/kernel.c#L707) | 707 | static | TTY固有の出力先へ1文字を書き込む． |
| [`tty_line_columns()`](../kernel/kernel.c#L727) | 727 | static | 正規モードでTABは0x09のまま保持し，エコー表示だけ4桁タブ位置へ展開する． |
| [`tty_foreground_interrupt()`](../kernel/kernel.c#L736) | 736 | static | Ctrl+Cで同じTTYの前景グループを終了させる． |
| [`tty_input_char()`](../kernel/kernel.c#L751) | 751 | static | TTYの入力モードに従い1文字を処理する． |
| [`wifi_session_stop()`](../kernel/kernel.c#L798) | 798 | static | 切断されたTelnetセッションのタスクとTTY状態を破棄する． |
| [`wifi_session_poll()`](../kernel/kernel.c#L814) | 814 | static | Telnetの接続状態・世代番号を監視して専用シェルを作る． |
| [`os_console_poll()`](../kernel/kernel.c#L848) | 848 | 外部 | アイドル側からUSB／Telnet入力，送信キュー，保留中のUART Ctrl+Cを処理する． |
| [`tty_rx_ready()`](../kernel/kernel.c#L875) | 875 | static | 共有TTY受信経路． |
| [`tty_rx_read()`](../kernel/kernel.c#L882) | 882 | static | 対象TTYの受信キューから読み取る． |
| [`wake_readers()`](../kernel/kernel.c#L888) | 888 | static | TTY入力待ちのタスクを，受信データと前景所有条件が揃った場合に再開させる． |
| [`uart1_irq_handler()`](../kernel/kernel.c#L909) | 909 | static | UART1の受信割込みを処理し，入力文字をTTYバッファへ蓄える． |
| [`pend()`](../kernel/kernel.c#L920) | 920 | static | PendSVを保留状態にする． |
| [`task_return()`](../kernel/kernel.c#L923) | 923 | static | タスクの入口関数がreturnした場合の終了処理へ誘導する． |
| [`os_init()`](../kernel/kernel.c#L926) | 926 | 外部 | タスク表，FD，TTY，固定メモリ管理と割込みを初期化する． |
| [`spawn_in_slot()`](../kernel/kernel.c#L952) | 952 | static | 指定スロットにPICイメージを複写し，argc/argvと初期レジスタフレームを配置する． |
| [`os_init_idle()`](../kernel/kernel.c#L1013) | 1013 | 外部 | アイドルタスク専用スタックと初期例外フレームを用意する． |
| [`os_idle_stack_used_bytes()`](../kernel/kernel.c#L1029) | 1029 | 外部 | スタックに埋めた0xA5の残量から使用量を推定する． |
| [`os_idle_stack_capacity_bytes()`](../kernel/kernel.c#L1036) | 1036 | 外部 | アイドルタスク専用スタックの確保サイズをバイト単位で返す． |
| [`os_spawn_named()`](../kernel/kernel.c#L1039) | 1039 | 外部 | スケジューラ開始前に指定の入口関数をタスクとして登録する． |
| [`os_register_program()`](../kernel/kernel.c#L1048) | 1048 | 外部 | プログラムID・名前・入口を起動前の表へ登録する． |
| [`os_start_program()`](../kernel/kernel.c#L1054) | 1054 | 外部 | 登録済みプログラムを起動前の空きスロットへ配置してPIDを返す． |
| [`os_spawn()`](../kernel/kernel.c#L1061) | 1061 | 外部 | 起動前に無名タスクを登録する簡易入口． |
| [`isr_systick()`](../kernel/kernel.c#L1064) | 1064 | 外部 | 1 ms周期の時刻更新と睡眠解除を行う． |
| [`os_schedule()`](../kernel/kernel.c#L1086) | 1086 | 外部 | PendSVが保存した旧PSPを受け，次のタスクのPSPを返す． |
| [`os_start()`](../kernel/kernel.c#L1140) | 1140 | 外部 | SysTickを1 msに設定し，最初のPendSVでタスク実行を開始する． |
| [`slot_from_pid()`](../kernel/kernel.c#L1152) | 1152 | static | 有効なPIDを持つタスクのスロット番号を探す． |
| [`in_task_range()`](../kernel/kernel.c#L1159) | 1159 | static | ユーザポインタpからnバイトが現在のスロット内に収まるか検査する． |
| [`path_match()`](../kernel/kernel.c#L1167) | 1167 | static | この関数へ渡すパスはカーネル側で正規化済みであり，ユーザポインタの検査は呼出側で終えている． |
| [`tmp_payload()`](../kernel/kernel.c#L1181) | 1181 | static | RAMファイル番号から固定サイズのデータ領域先頭アドレスを計算する． |
| [`tmp_name()`](../kernel/kernel.c#L1187) | 1187 | static | /tmpまたは/etc直下の名前を検証して内部形式へ変換する． |
| [`tmp_init()`](../kernel/kernel.c#L1204) | 1204 | static | Flashに埋め込まれた初期ファイルを起動後のRAMファイル領域へ一度だけ展開する． |
| [`tmp_lookup()`](../kernel/kernel.c#L1219) | 1219 | static | /tmpにあるRAMファイルを名前で検索し，該当番号または-1を返す． |
| [`free_fd()`](../kernel/kernel.c#L1226) | 1226 | static | ユーザがopenするFDは3以降． |
| [`proc_device()`](../kernel/kernel.c#L1232) | 1232 | static | デバイスIDが/procの仮想ファイルを表すか判定する． |
| [`tmp_open()`](../kernel/kernel.c#L1242) | 1242 | static | 固定11個のRAMファイルスロットから対象を探し，必要なら作成する． |
| [`tmp_unlink()`](../kernel/kernel.c#L1267) | 1267 | static | RAMファイルを削除する． |
| [`tmp_rw()`](../kernel/kernel.c#L1281) | 1281 | static | ファイルの現在位置を使って読み書きする． |
| [`absolute_path()`](../kernel/kernel.c#L1304) | 1304 | static | タスクごとのcwdを使って相対パスを正規化する． |
| [`valid_directory()`](../kernel/kernel.c#L1341) | 1341 | static | VFSが公開する固定ディレクトリか調べる． |
| [`vdir_nth()`](../kernel/kernel.c#L1347) | 1347 | static | 仮想ディレクトリのnth番目の項目を計算する． |
| [`vdir_entry_device_id()`](../kernel/kernel.c#L1425) | 1425 | static | ディレクトリエントリの表示IDを，プログラム／デバイス／RAMファイルの識別子に対応させる． |
| [`vfs_getdents()`](../kernel/kernel.c#L1460) | 1460 | static | ディレクトリFDの現在位置から固定長エントリを返す． |
| [`vfs_open()`](../kernel/kernel.c#L1472) | 1472 | static | 正規化済み絶対パスをデバイス／ディレクトリ／RAMファイルへ振り分け，FDを割り当てる． |
| [`device_rw_rand()`](../kernel/kernel.c#L1538) | 1538 | static | /dev/randomの読出し． |
| [`proc_emit()`](../kernel/kernel.c#L1557) | 1557 | static | /proc出力の位置と要求バイト数を考慮して，文字列の一部分だけをコピーする． |
| [`proc_tasks_read()`](../kernel/kernel.c#L1573) | 1573 | static | 生存タスクの概要を/proc/tasks形式へ逐次生成する． |
| [`proc_fb_read()`](../kernel/kernel.c#L1595) | 1595 | static | /procの8×8表示状況をテキストで返す． |
| [`fd_path()`](../kernel/kernel.c#L1614) | 1614 | static | FDの表示名はカーネルが管理するデバイスIDから求める． |
| [`proc_task_read()`](../kernel/kernel.c#L1657) | 1657 | static | 指定スロットのタスク詳細とFDを表示用テキストへ整形する． |
| [`proc_ramfiles_read()`](../kernel/kernel.c#L1707) | 1707 | static | 使用中RAMファイルの実データアドレスと名前を列挙する． |
| [`device_rw_proc()`](../kernel/kernel.c#L1729) | 1729 | static | /procの読み取りを担当する． |
| [`device_rw_tty()`](../kernel/kernel.c#L1783) | 1783 | static | TTYに対するread/writeを実装する． |
| [`device_rw_pwm()`](../kernel/kernel.c#L1824) | 1824 | static | /dev/pwmの文字列読書き． |
| [`device_rw_adc()`](../kernel/kernel.c#L1847) | 1847 | static | ADCを数値文字列として読む． |
| [`vfs_rw()`](../kernel/kernel.c#L1868) | 1868 | static | read/writeの共通入口． |
| [`device_ioctl_tty()`](../kernel/kernel.c#L2039) | 2039 | static | 端末の所有権と行入力・エコー・改行変換を管理する． |
| [`device_ioctl_gpio_status()`](../kernel/kernel.c#L2086) | 2086 | static | GPIO値のFDから，各ピンの状態を問い合わせられる． |
| [`device_ioctl()`](../kernel/kernel.c#L2109) | 2109 | static | ioctlの共通入口． |
| [`os_svc_dispatch()`](../kernel/kernel.c#L2151) | 2151 | 外部 | SVC例外で積まれたr0〜r3を取り出し，番号ごとの処理結果をr0へ返す． |
| [`call()`](../kernel/kernel.c#L2578) | 2578 | static | SVCディスパッチャへ渡す引数は明示的にレジスタへ割り当てる． |
| [`os_waitpid()`](../kernel/kernel.c#L2588) | 2588 | 外部 | SVCでwaitpidを実行するカーネル側ラッパー． |
| [`os_set_foreground()`](../kernel/kernel.c#L2593) | 2593 | 外部 | SVCで指定PIDをTTYの前景プロセスへ設定する． |
| [`os_spawn_program()`](../kernel/kernel.c#L2596) | 2596 | 外部 | SVCで登録済みのプログラムIDを起動する． |
| [`os_getpid()`](../kernel/kernel.c#L2599) | 2599 | 外部 | SVCを呼び出し，現在実行中のPIDを取得する． |
| [`os_get_tty()`](../kernel/kernel.c#L2602) | 2602 | 外部 | SVCを呼び出し，現在のタスクが所属するTTY番号を取得する． |
| [`os_kill()`](../kernel/kernel.c#L2605) | 2605 | 外部 | SVCで指定PIDのタスクの終了を要求する． |
| [`os_exit()`](../kernel/kernel.c#L2608) | 2608 | 外部 | SVCで現在のタスクを終了する． |
| [`os_msleep()`](../kernel/kernel.c#L2611) | 2611 | 外部 | SVCで指定ミリ秒だけタスクを睡眠状態へ移す． |
| [`os_yield()`](../kernel/kernel.c#L2614) | 2614 | 外部 | SVCで自発的にCPU実行権を譲る． |
| [`os_open()`](../kernel/kernel.c#L2617) | 2617 | 外部 | SVCでVFSのパスを開き，FDまたはエラー値を返す． |
| [`os_close()`](../kernel/kernel.c#L2620) | 2620 | 外部 | SVCで指定FDを閉じ，必要な端点参照を解放する． |
| [`os_read()`](../kernel/kernel.c#L2623) | 2623 | 外部 | SVCでFDから最大nバイトを読み取る． |
| [`os_write()`](../kernel/kernel.c#L2626) | 2626 | 外部 | SVCでFDに最大nバイトを書き込む． |
| [`os_ioctl()`](../kernel/kernel.c#L2629) | 2629 | 外部 | SVCでデバイス固有操作を要求する． |
| [`os_fcntl()`](../kernel/kernel.c#L2632) | 2632 | 外部 | SVCでFDフラグ等を取得・変更する． |
| [`os_pipe()`](../kernel/kernel.c#L2635) | 2635 | 外部 | SVCでパイプを作成し，読取端と書込端のFDを配列へ返す． |
| [`os_dup()`](../kernel/kernel.c#L2638) | 2638 | 外部 | SVCでFDを複製し，共有する端点への参照を増やす． |
| [`os_dup2()`](../kernel/kernel.c#L2641) | 2641 | 外部 | SVCでFDを指定番号へ複製する． |
| [`os_unlink()`](../kernel/kernel.c#L2644) | 2644 | 外部 | SVCでRAMファイルの指定パスを削除する． |
| [`os_socket()`](../kernel/kernel.c#L2647) | 2647 | 外部 | TCPクライアント・サーバとUDP通信を提供するソケット関連システムコール． |
| [`os_tcp_listen()`](../kernel/kernel.c#L2650) | 2650 | 外部 | SVCで指定FDをTCPの待受に設定する． |
| [`os_tcp_connect()`](../kernel/kernel.c#L2653) | 2653 | 外部 | SVCで指定FDのTCP接続を開始する． |
| [`os_tcp_status()`](../kernel/kernel.c#L2656) | 2656 | 外部 | SVCでTCP接続の進行・確立・異常を問い合わせる． |
| [`os_tcp_accept()`](../kernel/kernel.c#L2659) | 2659 | 外部 | SVCで待受FDから確立済みの接続を受け取り，新しいFDを得る． |

### `kernel/main.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`idle_task()`](../kernel/main.c#L13) | 13 | static | ユーザタスクが動かない間も，通信とUSB端末を継続処理する． |
| [`register_image()`](../kernel/main.c#L74) | 74 | static | Flash内のバイナリ範囲をプログラムIDに紐づける． |
| [`main()`](../kernel/main.c#L83) | 83 | 外部 | OSと各ユーザプログラムを登録し，初期タスクを配置してスケジューラへ制御を渡す． |

### `kernel/net.c`（27件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`wifi_cache_mac()`](../kernel/net.c#L36) | 36 | static | CYW43から取得したMACアドレスをキャッシュする． |
| [`wifi_mac_text()`](../kernel/net.c#L46) | 46 | static | 以前取得したMACアドレスを表示文字列にする． |
| [`net_poll_in_progress()`](../kernel/net.c#L70) | 70 | 外部 | lwIPのポーリング処理中か返す． |
| [`sta_rssi_clear()`](../kernel/net.c#L88) | 88 | static | STAのRSSIキャッシュを無効化し，次回の取得を許可する． |
| [`sta_rssi_poll()`](../kernel/net.c#L94) | 94 | static | STA接続時のRSSIを低頻度に取得し，診断表示にはキャッシュを使う． |
| [`sta_rssi_text()`](../kernel/net.c#L108) | 108 | static | RSSIを使った電波強度の推定値であり，通信品質の実測値ではない． |
| [`sta_mdns_remove()`](../kernel/net.c#L126) | 126 | static | STA側のmDNS登録を取り消し，再接続時の重複登録を避ける． |
| [`sta_mdns_ready()`](../kernel/net.c#L135) | 135 | static | IPアドレス確立後のmDNS提供状態を管理する． |
| [`sta_link_name()`](../kernel/net.c#L169) | 169 | static | CYW43のリンク状態コードを診断表示用の文字列に変換する． |
| [`sta_reset_association()`](../kernel/net.c#L183) | 183 | static | STA接続の状態と関連する情報を初期化して再接続可能にする． |
| [`sta_begin_connect()`](../kernel/net.c#L195) | 195 | static | STA接続を非同期に開始する． |
| [`sta_poll()`](../kernel/net.c#L210) | 210 | static | STAのリンク・DHCP取得と再接続の状態遷移を進める． |
| [`sta_start_services()`](../kernel/net.c#L284) | 284 | static | STAのリンク確立後にネットワークサービスとmDNS等を起動する． |
| [`dhcp_expire_offer()`](../kernel/net.c#L322) | 322 | static | 未完了のDHCP交渉が次の端末の接続を永久に妨げないようにする． |
| [`dhcp_recv()`](../kernel/net.c#L329) | 329 | static | AP側のDHCP/BOOTP要求を検査する． |
| [`ap_dhcp_server_start()`](../kernel/net.c#L412) | 412 | static | APモードのDHCPサービスを開始し，固定長の情報でアドレスを配布する． |
| [`net_init()`](../kernel/net.c#L432) | 432 | 外部 | Wi-Fi/LwIPを初期化する． |
| [`dhcp_send_pending()`](../kernel/net.c#L488) | 488 | static | cyw43_arch_pollとlwIPコールバックの外側から呼ぶ． |
| [`net_poll()`](../kernel/net.c#L520) | 520 | 外部 | アイドルタスクから無線・TCP/IP・保留送信・ループバックを順次処理する． |
| [`net_init()`](../kernel/net.c#L563) | 563 | 外部 | Wi-Fi/LwIPを初期化する． |
| [`net_poll()`](../kernel/net.c#L566) | 566 | 外部 | アイドルタスクから無線・TCP/IP・保留送信・ループバックを順次処理する． |
| [`net_poll_in_progress()`](../kernel/net.c#L569) | 569 | 外部 | 通常Picoビルドでもスケジューラから呼ばれる． |
| [`net_memory_info()`](../kernel/net.c#L573) | 573 | 外部 | lwIPヒープやプールの診断情報をテキスト化する． |
| [`net_wifi_info()`](../kernel/net.c#L599) | 599 | 外部 | /proc/net/wifi用の状態をテキストへ整形する． |
| [`net_wifi_info()`](../kernel/net.c#L651) | 651 | 外部 | /proc/net/wifi用の状態をテキストへ整形する． |
| [`net_wifi_request_restart()`](../kernel/net.c#L662) | 662 | 外部 | 再起動要求のフラグを立てる． |
| [`net_diag_info()`](../kernel/net.c#L673) | 673 | 外部 | AP/ARPの診断データを取得する． |

### `kernel/net_capture.c`（23件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`be16()`](../kernel/net_capture.c#L34) | 34 | static | パケット上のビッグエンディアン16ビット値を取り出す． |
| [`observe()`](../kernel/net_capture.c#L37) | 37 | static | ネットワークパケットの要約を固定長履歴へ記録する． |
| [`piconix_packet_input()`](../kernel/net_capture.c#L75) | 75 | 外部 | IPv4入力パケットを観測し，通常のlwIP受信処理は継続させる． |
| [`observed_output()`](../kernel/net_capture.c#L82) | 82 | static | lwIPの送信フックでパケットを記録し，元の送信関数を呼ぶ． |
| [`net_capture_attach()`](../kernel/net_capture.c#L91) | 91 | 外部 | 観測フックをネットワークIFへ接続する． |
| [`net_packet_serial()`](../kernel/net_capture.c#L102) | 102 | 外部 | パケット観測リングへ記録した通算番号を返す． |
| [`net_packet_reader()`](../kernel/net_capture.c#L105) | 105 | 外部 | 観測デバイスの読取利用数を加減する． |
| [`net_packet_read()`](../kernel/net_capture.c#L108) | 108 | 外部 | パケット記録を指定位置から読み出し，連続した観測に使う． |
| [`echo_received()`](../kernel/net_capture.c#L117) | 117 | static | ICMP Echo応答を検査し，待機している要求と対応する場合に結果を保存する． |
| [`net_capture_init()`](../kernel/net_capture.c#L137) | 137 | 外部 | ICMP Echo応答を受け取るraw PCBを初期化する． |
| [`net_echo_write()`](../kernel/net_capture.c#L143) | 143 | 外部 | ICMP Echo要求をネットワーク側へ送る． |
| [`net_echo_read()`](../kernel/net_capture.c#L151) | 151 | 外部 | 受信済みEcho応答を取り出す． |
| [`net_echo_close()`](../kernel/net_capture.c#L159) | 159 | 外部 | 未完了のEcho要求・応答状態を消去する． |
| [`net_capture_poll()`](../kernel/net_capture.c#L162) | 162 | 外部 | Echo待機のタイムアウトを確認し，応答状態を更新する． |
| [`net_capture_init()`](../kernel/net_capture.c#L193) | 193 | 外部 | ICMP Echo応答を受け取るraw PCBを初期化する． |
| [`net_capture_poll()`](../kernel/net_capture.c#L196) | 196 | 外部 | Echo待機のタイムアウトを確認し，応答状態を更新する． |
| [`net_capture_attach()`](../kernel/net_capture.c#L199) | 199 | 外部 | 観測フックをネットワークIFへ接続する． |
| [`net_packet_serial()`](../kernel/net_capture.c#L202) | 202 | 外部 | パケット観測リングへ記録した通算番号を返す． |
| [`net_packet_reader()`](../kernel/net_capture.c#L205) | 205 | 外部 | 観測デバイスの読取利用数を加減する． |
| [`net_packet_read()`](../kernel/net_capture.c#L208) | 208 | 外部 | パケット記録を指定位置から読み出し，連続した観測に使う． |
| [`net_echo_write()`](../kernel/net_capture.c#L211) | 211 | 外部 | ICMP Echo要求をネットワーク側へ送る． |
| [`net_echo_read()`](../kernel/net_capture.c#L214) | 214 | 外部 | 受信済みEcho応答を取り出す． |
| [`net_echo_close()`](../kernel/net_capture.c#L217) | 217 | 外部 | 未完了のEcho要求・応答状態を消去する． |

### `kernel/net_service_remote_devctl.c`（8件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`net_device_request()`](../kernel/net_service_remote_devctl.c#L13) | 13 | static | UDPで届いたデバイス操作要求を検査し，対応するGPIO等の操作へ変換する． |
| [`device_recv()`](../kernel/net_service_remote_devctl.c#L109) | 109 | static | ネットワークコールバックで要求を受け取る． |
| [`device_start()`](../kernel/net_service_remote_devctl.c#L122) | 122 | static | UDP 2000番のデバイス制御受信口を初期化し，コールバックを設定する． |
| [`device_process_pending()`](../kernel/net_service_remote_devctl.c#L133) | 133 | static | 保留したデバイス要求をアイドル文脈で処理する． |
| [`net_service_remote_devctl_init()`](../kernel/net_service_remote_devctl.c#L149) | 149 | 外部 | デバイス制御サービスを初期化する． |
| [`net_service_remote_devctl_poll()`](../kernel/net_service_remote_devctl.c#L152) | 152 | 外部 | 受信済みのデバイス制御要求を処理する． |
| [`net_service_remote_devctl_init()`](../kernel/net_service_remote_devctl.c#L155) | 155 | 外部 | デバイス制御サービスを初期化する． |
| [`net_service_remote_devctl_poll()`](../kernel/net_service_remote_devctl.c#L158) | 158 | 外部 | 受信済みのデバイス制御要求を処理する． |

### `kernel/net_service_telnet.c`（20件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`queue_count()`](../kernel/net_service_telnet.c#L30) | 30 | static | Telnetのリングキュー内に存在するバイト数を計算する． |
| [`queue_space()`](../kernel/net_service_telnet.c#L35) | 35 | static | キュー末尾を1つ空けたうえで書き込めるバイト数を返す． |
| [`net_queues_reset()`](../kernel/net_service_telnet.c#L40) | 40 | static | Telnetの送受信キューを空にし，新規接続へ備える． |
| [`net_terminal_connected()`](../kernel/net_service_telnet.c#L45) | 45 | 外部 | Telnetクライアントの接続有無を返す． |
| [`net_terminal_epoch()`](../kernel/net_service_telnet.c#L48) | 48 | 外部 | 端末接続の世代番号を返す． |
| [`net_terminal_read()`](../kernel/net_service_telnet.c#L51) | 51 | 外部 | 受信キューから指定した最大バイト数を取り出す． |
| [`net_terminal_write()`](../kernel/net_service_telnet.c#L63) | 63 | 外部 | TTY送信バイトを固定長キューに積み，積めた量を返す． |
| [`net_terminal_flush()`](../kernel/net_service_telnet.c#L87) | 87 | static | ポーリング文脈からネットワークへ送信キューを流す． |
| [`client_error()`](../kernel/net_service_telnet.c#L114) | 114 | static | lwIPが通知したTCP異常を受け，破棄済みPCBへの参照を外す． |
| [`net_receive_byte()`](../kernel/net_service_telnet.c#L123) | 123 | static | Telnetの制御列と通常入力を分ける． |
| [`client_recv()`](../kernel/net_service_telnet.c#L154) | 154 | static | TCPコールバックで受信を固定キューへ入れる． |
| [`client_accept()`](../kernel/net_service_telnet.c#L193) | 193 | static | 単一のTelnetクライアントを受け入れ，TTY用の新しいセッション番号を与える． |
| [`net_service_telnet_init()`](../kernel/net_service_telnet.c#L220) | 220 | 外部 | TCP 23番のリスナを用意する． |
| [`net_service_telnet_poll()`](../kernel/net_service_telnet.c#L239) | 239 | 外部 | 保留されたTelnet送信と切断処理をアイドル側で進める． |
| [`net_service_telnet_init()`](../kernel/net_service_telnet.c#L242) | 242 | 外部 | TCP 23番のリスナを用意する． |
| [`net_service_telnet_poll()`](../kernel/net_service_telnet.c#L245) | 245 | 外部 | 保留されたTelnet送信と切断処理をアイドル側で進める． |
| [`net_terminal_connected()`](../kernel/net_service_telnet.c#L248) | 248 | 外部 | Telnetクライアントの接続有無を返す． |
| [`net_terminal_epoch()`](../kernel/net_service_telnet.c#L251) | 251 | 外部 | 端末接続の世代番号を返す． |
| [`net_terminal_read()`](../kernel/net_service_telnet.c#L254) | 254 | 外部 | 受信キューから指定した最大バイト数を取り出す． |
| [`net_terminal_write()`](../kernel/net_service_telnet.c#L257) | 257 | 外部 | TTY送信バイトを固定長キューに積み，積めた量を返す． |

### `kernel/net_socket.c`（44件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`udp_received()`](../kernel/net_socket.c#L22) | 22 | static | UDP受信データを固定の一件バッファへ保存する． |
| [`net_socket_udp_bind()`](../kernel/net_socket.c#L36) | 36 | 外部 | UDP待受ポートをlwIPへ登録する． |
| [`net_socket_udp_recv()`](../kernel/net_socket.c#L51) | 51 | 外部 | 保持中の一つのデータグラムを取り出す． |
| [`net_socket_udp_send()`](../kernel/net_socket.c#L66) | 66 | 外部 | 宛先IPv4とポートを指定してUDPデータグラムを送る． |
| [`net_socket_udp_close()`](../kernel/net_socket.c#L91) | 91 | 外部 | UDPソケットを解放し，受信状態をリセットする． |
| [`slot_at()`](../kernel/net_socket.c#L108) | 108 | static | IDが有効な範囲にある場合だけ固定TCPスロットのポインタを返す． |
| [`net_socket_tcp_alloc()`](../kernel/net_socket.c#L113) | 113 | 外部 | 固定TCP管理スロットを確保する． |
| [`socket_tcp_connected_cb()`](../kernel/net_socket.c#L124) | 124 | static | 非同期TCP接続の完了を受け，接続状態とエラーを更新する． |
| [`socket_tcp_err()`](../kernel/net_socket.c#L134) | 134 | static | lwIPがTCP PCBを破棄したときに管理スロットの参照を解除する． |
| [`socket_tcp_recv()`](../kernel/net_socket.c#L145) | 145 | static | TCP受信コールバック． |
| [`socket_tcp_accept()`](../kernel/net_socket.c#L160) | 160 | static | 受け付けたTCP接続を管理スロットに登録し，accept側へ新しい接続の到着を通知する． |
| [`net_socket_tcp_listen()`](../kernel/net_socket.c#L175) | 175 | 外部 | 指定スロットでTCP待受を始める． |
| [`net_socket_tcp_connect()`](../kernel/net_socket.c#L192) | 192 | 外部 | TCP接続を非同期に開始する． |
| [`net_socket_tcp_accept_ready()`](../kernel/net_socket.c#L215) | 215 | 外部 | 待受スロットに受け取り可能な接続があるか確認する． |
| [`net_socket_tcp_listening()`](../kernel/net_socket.c#L223) | 223 | 外部 | 指定TCPスロットが待受状態なら真を返す． |
| [`net_socket_tcp_close_listener()`](../kernel/net_socket.c#L226) | 226 | 外部 | TCP待受PCBを閉じる． |
| [`socket_tcp_closing_poll()`](../kernel/net_socket.c#L236) | 236 | static | 保留したtcp_closeはTCPタイマーコールバックからだけ再試行する． |
| [`socket_tcp_closing_recv()`](../kernel/net_socket.c#L243) | 243 | static | 終了処理中に到着した受信バッファを破棄して受信窓を進める． |
| [`net_socket_tcp_close_client()`](../kernel/net_socket.c#L251) | 251 | 外部 | アプリから切り離したTCP接続を閉じる． |
| [`net_socket_tcp_connected()`](../kernel/net_socket.c#L270) | 270 | 外部 | 指定TCPスロットの接続が確立済みか返す． |
| [`net_socket_tcp_status()`](../kernel/net_socket.c#L273) | 273 | 外部 | TCP接続の状態を返す． |
| [`net_socket_tcp_read()`](../kernel/net_socket.c#L282) | 282 | 外部 | 固定受信キューから最大capacityバイトをコピーする． |
| [`net_socket_tcp_write()`](../kernel/net_socket.c#L297) | 297 | 外部 | 送信バッファとMSSに収まる範囲だけをlwIPへ渡す． |
| [`net_socket_tcp_shutdown_write()`](../kernel/net_socket.c#L315) | 315 | 外部 | TCPの送信方向だけを終了し，受信方向は維持する． |
| [`net_socket_tcp_close()`](../kernel/net_socket.c#L324) | 324 | 外部 | 指定TCPスロットのクライアントと待受を閉じ，管理スロットを解放する． |
| [`net_socket_local_port()`](../kernel/net_socket.c#L332) | 332 | 外部 | UDPまたはTCPの指定ソケットに割り当てられたローカルポートを返す． |
| [`net_socket_udp_bind()`](../kernel/net_socket.c#L343) | 343 | 外部 | UDP待受ポートをlwIPへ登録する． |
| [`net_socket_udp_recv()`](../kernel/net_socket.c#L346) | 346 | 外部 | 保持中の一つのデータグラムを取り出す． |
| [`net_socket_udp_send()`](../kernel/net_socket.c#L351) | 351 | 外部 | 宛先IPv4とポートを指定してUDPデータグラムを送る． |
| [`net_socket_udp_close()`](../kernel/net_socket.c#L356) | 356 | 外部 | UDPソケットを解放し，受信状態をリセットする． |
| [`net_socket_tcp_alloc()`](../kernel/net_socket.c#L359) | 359 | 外部 | 固定TCP管理スロットを確保する． |
| [`net_socket_tcp_listen()`](../kernel/net_socket.c#L362) | 362 | 外部 | 指定スロットでTCP待受を始める． |
| [`net_socket_tcp_accept_ready()`](../kernel/net_socket.c#L365) | 365 | 外部 | 待受スロットに受け取り可能な接続があるか確認する． |
| [`net_socket_tcp_listening()`](../kernel/net_socket.c#L368) | 368 | 外部 | 指定TCPスロットが待受状態なら真を返す． |
| [`net_socket_tcp_close_listener()`](../kernel/net_socket.c#L371) | 371 | 外部 | TCP待受PCBを閉じる． |
| [`net_socket_tcp_close_client()`](../kernel/net_socket.c#L374) | 374 | 外部 | アプリから切り離したTCP接続を閉じる． |
| [`net_socket_tcp_connected()`](../kernel/net_socket.c#L377) | 377 | 外部 | 指定TCPスロットの接続が確立済みか返す． |
| [`net_socket_tcp_status()`](../kernel/net_socket.c#L380) | 380 | 外部 | TCP接続の状態を返す． |
| [`net_socket_tcp_connect()`](../kernel/net_socket.c#L383) | 383 | 外部 | TCP接続を非同期に開始する． |
| [`net_socket_tcp_read()`](../kernel/net_socket.c#L386) | 386 | 外部 | 固定受信キューから最大capacityバイトをコピーする． |
| [`net_socket_tcp_write()`](../kernel/net_socket.c#L389) | 389 | 外部 | 送信バッファとMSSに収まる範囲だけをlwIPへ渡す． |
| [`net_socket_tcp_shutdown_write()`](../kernel/net_socket.c#L392) | 392 | 外部 | TCPの送信方向だけを終了し，受信方向は維持する． |
| [`net_socket_tcp_close()`](../kernel/net_socket.c#L395) | 395 | 外部 | 指定TCPスロットのクライアントと待受を閉じ，管理スロットを解放する． |
| [`net_socket_local_port()`](../kernel/net_socket.c#L398) | 398 | 外部 | UDPまたはTCPの指定ソケットに割り当てられたローカルポートを返す． |

### `kernel/pins.h`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`pin_mode()`](../kernel/pins.h#L122) | 122 | static | ボードごとの固定ピン設定表から，GPIO／ADC／PWM等の利用モードを返す． |
| [`pin_gpio()`](../kernel/pins.h#L130) | 130 | static | 指定ピンが汎用GPIOとして利用可能なら真を返す． |
| [`pin_adc()`](../kernel/pins.h#L133) | 133 | static | 指定チャネルに対応するGPIOがADC用なら真を返す． |
| [`pin_pwm()`](../kernel/pins.h#L136) | 136 | static | GP18／GP19がPWM用に割り当てられているか確認する． |
| [`pin_any_pwm()`](../kernel/pins.h#L139) | 139 | static | PWMに割り当てたピンが1本以上あるか返す． |

### `kernel/pwm.c`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`apply_pwm()`](../kernel/pwm.c#L14) | 14 | static | 周波数とデューティ比をRP2040のPWMレジスタへ反映する． |
| [`pwm_driver_init()`](../kernel/pwm.c#L34) | 34 | 外部 | PWMピンの機能を設定し，既定の周波数とデューティ比を適用する． |
| [`pwm_driver_get()`](../kernel/pwm.c#L41) | 41 | 外部 | 0は周波数，1・2は各チャネルのデューティ比を返す． |
| [`pwm_driver_set()`](../kernel/pwm.c#L44) | 44 | 外部 | 周波数またはデューティ比を検証して設定する． |
| [`pwm_parse_value()`](../kernel/pwm.c#L52) | 52 | 外部 | ローカルVFS書込みとUDPデバイス制御で共用する処理． |

## ユーザプログラムとライブラリ

### `userland/apps/asm.c`（12件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`space()`](../userland/apps/asm.c#L23) | 23 | static | 入力文字列の先頭にある空白・タブを読み飛ばし，次の文字位置を返す． |
| [`end()`](../userland/apps/asm.c#L26) | 26 | static | 行末側の空白・タブ・復帰文字を削除し，字句解析を簡単にする． |
| [`number()`](../userland/apps/asm.c#L29) | 29 | static | 10進・16進の即値を解析する． |
| [`reg()`](../userland/apps/asm.c#L41) | 41 | static | レジスタ表記r0〜r7を検証し，番号または-1を返す． |
| [`registers()`](../userland/apps/asm.c#L47) | 47 | static | push／popのレジスタ集合をビットマスクへ変換する． |
| [`memory_operand()`](../userland/apps/asm.c#L65) | 65 | static | [rN, 即値]形式のメモリオペランドから基底レジスタとオフセットを求める． |
| [`label()`](../userland/apps/asm.c#L75) | 75 | static | 1パス目で登録したラベル名を探し，コード内の相対位置を返す． |
| [`emit16()`](../userland/apps/asm.c#L78) | 78 | static | Thumb命令を16ビット単位で出力する． |
| [`byte()`](../userland/apps/asm.c#L81) | 81 | static | 1バイトをコードへ追加する． |
| [`compile()`](../userland/apps/asm.c#L84) | 84 | static | ソースを1パスずつ解析する． |
| [`write_all()`](../userland/apps/asm.c#L180) | 180 | static | FDへ全バイトを書き終えるまでwriteを繰り返す． |
| [`main()`](../userland/apps/asm.c#L183) | 183 | 外部 | Thumb-1アセンブリのファイルを読み，2パスで変換して実行用イメージを出力する． |

### `userland/apps/bh1750.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`transfer()`](../userland/apps/bh1750.c#L18) | 18 | static | /dev/i2c/1を使い，BH1750へ指定バイトを書いて測定値を受け取る． |
| [`main()`](../userland/apps/bh1750.c#L38) | 38 | 外部 | BH1750照度センサをI2Cで操作し，取得した照度値を表示する． |

### `userland/apps/blink.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/blink.c#L10) | 10 | 外部 | GPIOを出力に設定する． |

### `userland/apps/cat.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`output()`](../userland/apps/cat.c#L11) | 11 | static | 標準出力への書込みを繰り返し，部分書込みと一時的な待機を処理する． |
| [`numbered_output()`](../userland/apps/cat.c#L23) | 23 | static | 32バイト単位のreadをまたいでも，空行を含む行頭位置を保持する． |
| [`display()`](../userland/apps/cat.c#L41) | 41 | static | 読み取ったデータ片を表示する． |
| [`main()`](../userland/apps/cat.c#L47) | 47 | 外部 | 指定ファイルまたは標準入力の内容を順番に表示する． |

### `userland/apps/cp.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`path()`](../userland/apps/cp.c#L11) | 11 | static | 自分自身をtruncateしないよう，コピー元とコピー先の相対パスを正規化する． |
| [`main()`](../userland/apps/cp.c#L44) | 44 | 外部 | 指定したファイルの内容を別ファイルへ複写する． |

### `userland/apps/dd.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`number()`](../userland/apps/dd.c#L12) | 12 | static | コマンド引数の10進数または0x付き16進数を解析する． |
| [`usage()`](../userland/apps/dd.c#L21) | 21 | static | ddで利用可能な入出力指定と転送オプションを表示する． |
| [`main()`](../userland/apps/dd.c#L24) | 24 | 外部 | 指定した入力FDから出力FDへデータをブロック単位でコピーする． |

### `userland/apps/echo.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`emit()`](../userland/apps/echo.c#L8) | 8 | static | 標準出力へ1文字を確実に書く． |
| [`digit()`](../userland/apps/echo.c#L18) | 18 | static | エスケープの16進数字を数値へ変換する． |
| [`output()`](../userland/apps/echo.c#L25) | 25 | static | エスケープ指定を考慮し，echoの引数文字列を出力する． |
| [`main()`](../userland/apps/echo.c#L55) | 55 | 外部 | 引数を空白区切りで表示し，オプションに応じて改行・エスケープを扱う． |

### `userland/apps/edit.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/edit.c#L12) | 12 | 外部 | 共通エディタ処理を使って編集対象を開き，キー入力で編集して保存する． |

### `userland/apps/editor_common.h`（15件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`width()`](../userland/apps/editor_common.h#L22) | 22 | static | 文字の画面表示幅を返す． |
| [`line_start()`](../userland/apps/editor_common.h#L25) | 25 | static | 指定位置を含む行の先頭オフセットを探す． |
| [`line_end()`](../userland/apps/editor_common.h#L28) | 28 | static | 指定位置を含む行の改行手前のオフセットを探す． |
| [`line_no()`](../userland/apps/editor_common.h#L31) | 31 | static | テキスト先頭から指定位置までの改行数を数え，行番号を返す． |
| [`col_no()`](../userland/apps/editor_common.h#L34) | 34 | static | 指定位置の表示桁をタブ展開込みで求める． |
| [`position_col()`](../userland/apps/editor_common.h#L37) | 37 | static | 行頭から指定表示桁まで進め，対応する文字オフセットを返す． |
| [`insert()`](../userland/apps/editor_common.h#L40) | 40 | static | カーソル位置に文字を挿入する． |
| [`erase()`](../userland/apps/editor_common.h#L43) | 43 | static | カーソル直前の文字を削除し，後続バイトを左へ詰める． |
| [`del()`](../userland/apps/editor_common.h#L46) | 46 | static | カーソル位置の文字を削除し，後続バイトを左へ詰める． |
| [`save()`](../userland/apps/editor_common.h#L49) | 49 | static | 編集バッファを指定RAMファイルへ保存する． |
| [`measure_screen()`](../userland/apps/editor_common.h#L52) | 52 | static | 端末サイズが極端に大きくても，描画出力は固定上限に収める． |
| [`paint_row()`](../userland/apps/editor_common.h#L61) | 61 | static | EDITOR_GUTTERはeditで行番号用の4桁，viでは0となる． |
| [`draw()`](../userland/apps/editor_common.h#L85) | 85 | static | 表示モードは0=全体，1=変更行，2=カーソルと状態のみ． |
| [`editor_normalize_path()`](../userland/apps/editor_common.h#L115) | 115 | static | 書込み可能なRAMパスを検査する前に「.」「..」を正規化する． |
| [`editor_start()`](../userland/apps/editor_common.h#L138) | 138 | static | パス解決・2 KiBまでの読出し・TTY設定を共通化する． |

### `userland/apps/fbctl.c`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`same()`](../userland/apps/fbctl.c#L12) | 12 | static | 二つのコマンド文字列が等しいか判定する． |
| [`pattern()`](../userland/apps/fbctl.c#L15) | 15 | static | 8バイトのフレームバッファパターンをFDへ書き込む． |
| [`status()`](../userland/apps/fbctl.c#L18) | 18 | static | cat /proc/fbと同じカーネル生成レポートを表示する． |
| [`usage()`](../userland/apps/fbctl.c#L39) | 39 | static | LED表示の有効化・消灯・テスト操作の使用方法を表示する． |
| [`main()`](../userland/apps/fbctl.c#L51) | 51 | 外部 | /dev/gpio/fbを介して8×8 LED表示の状態確認や操作を行う． |

### `userland/apps/fbed.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`draw()`](../userland/apps/fbed.c#L11) | 11 | static | 編集中の8×8 LED画面とカーソルを端末へ描画する． |
| [`main()`](../userland/apps/fbed.c#L27) | 27 | 外部 | 端末から8×8の点灯パターンを編集し，フレームバッファへ反映する． |

### `userland/apps/fd.c`（15件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`reset_view_index()`](../userland/apps/fd.c#L34) | 34 | static | ファイル表示用の疎な行索引を先頭位置で初期化する． |
| [`index_line()`](../userland/apps/fd.c#L40) | 40 | static | 表示範囲内のファイル内容を行ごとに索引化する． |
| [`clipped()`](../userland/apps/fd.c#L51) | 51 | static | 指定幅以内で文字列を切り出し，必ず終端文字を付ける． |
| [`join()`](../userland/apps/fd.c#L58) | 58 | static | ディレクトリとエントリ名から絶対パスを組み立てる． |
| [`parent_dir()`](../userland/apps/fd.c#L69) | 69 | static | 現在の閲覧パスを親ディレクトリへ移動する． |
| [`directory_load()`](../userland/apps/fd.c#L77) | 77 | static | getdentsでディレクトリを列挙し，表示用の項目を保持する． |
| [`refresh_dir()`](../userland/apps/fd.c#L100) | 100 | static | 現在ディレクトリのエントリをVFSから読み直し，選択位置を更新する． |
| [`draw_dir()`](../userland/apps/fd.c#L103) | 103 | static | ファイル一覧と選択位置を端末画面へ表示する． |
| [`safe_view()`](../userland/apps/fd.c#L128) | 128 | static | 既知のテキスト系ファイルだけを閲覧する． |
| [`view_load()`](../userland/apps/fd.c#L134) | 134 | static | ファイルの対象範囲を読み取り，現在のページ表示を準備する． |
| [`paint_view()`](../userland/apps/fd.c#L200) | 200 | static | 現在のファイルページを端末へ描画する． |
| [`refresh_view()`](../userland/apps/fd.c#L227) | 227 | static | ファイルの表示領域を読み直す． |
| [`edit_selected()`](../userland/apps/fd.c#L239) | 239 | static | 選択したRAMファイルに対する編集コマンドを呼び出す． |
| [`key_event()`](../userland/apps/fd.c#L271) | 271 | static | キー入力からディレクトリ移動・閲覧等の操作を選択する． |
| [`main()`](../userland/apps/fd.c#L286) | 286 | 外部 | 端末を設定してファイルブラウザの操作ループを実行する． |

### `userland/apps/find.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`match()`](../userland/apps/find.c#L8) | 8 | static | ファイル名を*と?の簡易ワイルドカードに照合する． |
| [`visit()`](../userland/apps/find.c#L19) | 19 | static | 列挙したエントリを表示し，ディレクトリなら深さ制限内で再帰探索する． |
| [`descend()`](../userland/apps/find.c#L27) | 27 | static | 指定ディレクトリを巡回する． |
| [`main()`](../userland/apps/find.c#L34) | 34 | 外部 | 開始位置からVFSをたどり，条件に合うパスを表示する． |

### `userland/apps/gpio.c`（11件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`available()`](../userland/apps/gpio.c#L16) | 16 | static | 画面に表示するGPIO番号の範囲を確認し，未公開ピンを除外する． |
| [`at()`](../userland/apps/gpio.c#L19) | 19 | static | 行と列から，表形式表示に対応するGPIO番号を計算する． |
| [`editable()`](../userland/apps/gpio.c#L22) | 22 | static | GPIO状態がFREE・IN・OUTのいずれかで，操作を許可できるか判定する． |
| [`query()`](../userland/apps/gpio.c#L27) | 27 | static | カーネルのGPIO状態取得ioctlを呼び，指定ピンの状態値を得る． |
| [`owner()`](../userland/apps/gpio.c#L30) | 30 | static | 予約済みGPIO状態コードから使用機能の表示名を返す． |
| [`pin_ioctl()`](../userland/apps/gpio.c#L43) | 43 | static | 対象GPIOのデバイスを開いてioctlを実行し，FDを閉じる． |
| [`set_level()`](../userland/apps/gpio.c#L54) | 54 | static | GPIOデバイスに0／1を書き込んで出力レベルを設定する． |
| [`cell()`](../userland/apps/gpio.c#L66) | 66 | static | 1ピン分の所有者・方向・レベルを表示し，選択ピンを反転表示する． |
| [`draw()`](../userland/apps/gpio.c#L85) | 85 | static | GPIOを格子状に描画し，利用可否と現在の選択を示す． |
| [`navigate()`](../userland/apps/gpio.c#L106) | 106 | static | 表示グリッド順に移動し，利用不可ピンを飛ばし，既存の所有関係を変更しない． |
| [`main()`](../userland/apps/gpio.c#L119) | 119 | 外部 | GPIO状態を端末で対話的に表示・変更する． |

### `userland/apps/grep.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`emit()`](../userland/apps/grep.c#L11) | 11 | static | 見つかった行を標準出力へ転送する． |
| [`match()`](../userland/apps/grep.c#L21) | 21 | static | 指定した検索語が対象の行に含まれるかを調べる． |
| [`main()`](../userland/apps/grep.c#L32) | 32 | 外部 | 標準入力やファイルから条件に一致する行を抽出する． |

### `userland/apps/hello.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/hello.c#L8) | 8 | 外部 | PicoNIXの動作確認用メッセージを表示する． |

### `userland/apps/help.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/help.c#L9) | 9 | 外部 | 登録コマンドと利用方法の簡易案内を表示する． |

### `userland/apps/hexdump.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`number()`](../userland/apps/hexdump.c#L13) | 13 | static | オフセット表示などに用いる数値引数を解析する． |
| [`main()`](../userland/apps/hexdump.c#L24) | 24 | 外部 | 入力ファイルを16進数と文字の対応が分かる形式で表示する． |

### `userland/apps/httpd.c`（11件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`sendall()`](../userland/apps/httpd.c#L11) | 11 | static | TCP接続への送信を完了するまで繰り返す． |
| [`same()`](../userland/apps/httpd.c#L21) | 21 | static | HTTPのパスや操作名を文字列一致で判定する． |
| [`response()`](../userland/apps/httpd.c#L24) | 24 | static | HTTP/1.0の状態行・ヘッダ・本文を送る． |
| [`text_reply()`](../userland/apps/httpd.c#L33) | 33 | static | UTF-8プレーンテキストのHTTP応答を生成する． |
| [`hexval()`](../userland/apps/httpd.c#L36) | 36 | static | 16進数1桁を0〜15へ変換する． |
| [`pwm_path()`](../userland/apps/httpd.c#L39) | 39 | static | Web APIのPWMパスをVFSの/dev/pwmパスへ変換する． |
| [`api()`](../userland/apps/httpd.c#L47) | 47 | static | フレームバッファとPWM用HTTP APIを解析してデバイス操作へ振り分ける． |
| [`content_length()`](../userland/apps/httpd.c#L115) | 115 | static | ヘッダを一行ずつ検査し，曖昧・不正な長さを拒否する． |
| [`recv_timed()`](../userland/apps/httpd.c#L142) | 142 | static | ノンブロッキング通信ではOS_EAGAINは一時的な状態，0はTCP EOFである． |
| [`handle()`](../userland/apps/httpd.c#L149) | 149 | static | 固定長要求バッファでGETと小さなPOSTだけを扱い，chunked転送は実装しない． |
| [`main()`](../userland/apps/httpd.c#L212) | 212 | 外部 | TCP待受と要求処理を繰り返す． |

### `userland/apps/i2cctl.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`number()`](../userland/apps/i2cctl.c#L12) | 12 | static | I2Cアドレス・転送値の数値引数を解析する． |
| [`usage()`](../userland/apps/i2cctl.c#L19) | 19 | static | I2C制御コマンドの指定形式を表示する． |
| [`main()`](../userland/apps/i2cctl.c#L25) | 25 | 外部 | /dev/i2c/1のioctlを使い，指定デバイスへ書込み・読出しを行う． |

### `userland/apps/ip.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`field()`](../userland/apps/ip.c#L9) | 9 | static | /proc/net/wifiの行から指定キーの値を取り出し，固定長領域に格納する． |
| [`main()`](../userland/apps/ip.c#L25) | 25 | 外部 | ループバックとWi-Fiインターフェースのアドレス情報を読み取り専用で表示する． |

### `userland/apps/kill.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/kill.c#L8) | 8 | 外部 | 指定PIDに終了要求を送り，結果を表示する． |

### `userland/apps/kitesys.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`display()`](../userland/apps/kitesys.c#L24) | 24 | static | 数字フォントを8×8表示用の行データへ変換し，フレームバッファへ送る． |
| [`pressed()`](../userland/apps/kitesys.c#L32) | 32 | static | Lowで押下を検出し，30 msデバウンス後の安定した押下を一回だけ返す． |
| [`pwm_value()`](../userland/apps/kitesys.c#L50) | 50 | static | PWMデバイスへ数値文字列を送り，書込みの成否を返す． |
| [`main()`](../userland/apps/kitesys.c#L57) | 57 | 外部 | ボタンでLED表示位置を動かし，PWMブザーも鳴らす統合動作テストを行う． |

### `userland/apps/ls.c`（8件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`eq()`](../userland/apps/ls.c#L8) | 8 | static | 二つのファイル名・オプション文字列が等しいか判定する． |
| [`write_all()`](../userland/apps/ls.c#L11) | 11 | static | 標準出力への全バイトの書込みを行う． |
| [`put()`](../userland/apps/ls.c#L14) | 14 | static | 表示文字列を標準出力へ出力する． |
| [`number()`](../userland/apps/ls.c#L17) | 17 | static | 数値を表示用の文字列へ変換する． |
| [`resolve()`](../userland/apps/ls.c#L20) | 20 | static | 相対パスを現在ディレクトリからのパスへ変換する． |
| [`entry()`](../userland/apps/ls.c#L51) | 51 | static | デバイスはデバイスID，/binはプログラムID，RAMファイルはスロットIDを表示する． |
| [`ls()`](../userland/apps/ls.c#L76) | 76 | static | 指定ディレクトリまたはファイルの情報を読み取って一覧表示する． |
| [`main()`](../userland/apps/ls.c#L93) | 93 | 外部 | 指定パスの一覧を表示し，オプションに従って表示形式を切り替える． |

### `userland/apps/lsof.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`number()`](../userland/apps/lsof.c#L11) | 11 | static | /procの数値フィールドを読み，次の文字位置へ進める． |
| [`show_task()`](../userland/apps/lsof.c#L14) | 14 | static | /proc/task/PIDを解析して対象プロセスのFD一覧を表示する． |
| [`main()`](../userland/apps/lsof.c#L53) | 53 | 外部 | 指定PIDまたは全プロセスについて，開いているFDの内容を表示する． |

### `userland/apps/mb.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`draw()`](../userland/apps/mb.c#L18) | 18 | static | 複数行の月面バギーを固定位置に描く． |
| [`main()`](../userland/apps/mb.c#L55) | 55 | 外部 | 端末を非カノニカル・非ブロック入力に変更して月面バギーゲームを動かす． |

### `userland/apps/mem.c`（17件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`mem_layout()`](../userland/apps/mem.c#L21) | 21 | static | カーネル・タスク・共有RAM・ファイル・トレース・ネットワーク領域を表示用に区分する． |
| [`valid()`](../userland/apps/mem.c#L35) | 35 | static | 読み出し対象アドレスが公開されたSRAMまたはXIP Flash範囲にあるか確認する． |
| [`hex()`](../userland/apps/mem.c#L38) | 38 | static | 指定桁数の16進数を標準出力へ表示する． |
| [`fetch()`](../userland/apps/mem.c#L41) | 41 | static | /dev/memにseekして表示対象領域を取得する． |
| [`region()`](../userland/apps/mem.c#L44) | 44 | static | 物理アドレスをカーネル，タスク，RAMファイル等の表示領域名に対応付ける． |
| [`symbol_record()`](../userland/apps/mem.c#L56) | 56 | static | シンボル表の形式は「アプリ名 TAB 相対16進アドレス TAB 種別 TAB 名前 LF」である． |
| [`scan_symbols()`](../userland/apps/mem.c#L70) | 70 | static | シンボル表からアドレスと名前を読み，現在の表示位置と関連付ける． |
| [`task_program()`](../userland/apps/mem.c#L141) | 141 | static | /proc/tasksを現在の各RAMスロットの情報源とする． |
| [`refresh_slots()`](../userland/apps/mem.c#L185) | 185 | static | /procから生存タスクのスロット情報を取得して表示へ反映する． |
| [`append_app()`](../userland/apps/mem.c#L230) | 230 | static | 利用中スロットのプログラム名から表示ラベルを更新する． |
| [`refresh_ram_labels()`](../userland/apps/mem.c#L247) | 247 | static | 現在存在するRAMファイルの場所と名前を表示へ反映する． |
| [`refresh_labels()`](../userland/apps/mem.c#L271) | 271 | static | メモリアドレスと各領域の名称を再計算する． |
| [`draw()`](../userland/apps/mem.c#L318) | 318 | static | アドレス範囲と内容，タスク・シンボルを端末の大きさに合わせて描画する． |
| [`parse()`](../userland/apps/mem.c#L355) | 355 | static | 対話操作で入力されたアドレスや数値を解析する． |
| [`shifted()`](../userland/apps/mem.c#L358) | 358 | static | 表示するメモリ先頭アドレスを指定量だけ移動する． |
| [`list_symbol_sources()`](../userland/apps/mem.c#L365) | 365 | static | /proc/symの対象から選択可能なカーネル／アプリのシンボル表を列挙する． |
| [`main()`](../userland/apps/mem.c#L378) | 378 | 外部 | アドレス移動と表示切替を行う対話型メモリ閲覧を開始する． |

### `userland/apps/nc.c`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`port()`](../userland/apps/nc.c#L10) | 10 | static | ポート番号の文字列を検査し，16ビットの値へ変換する． |
| [`ipv4()`](../userland/apps/nc.c#L18) | 18 | static | lwIPのip4_addr_t.addrはuint32にネットワーク順の値として格納される． |
| [`send_all()`](../userland/apps/nc.c#L32) | 32 | static | TCP等の送信可能な範囲へ全データを書き込む． |
| [`usage()`](../userland/apps/nc.c#L39) | 39 | static | ncのTCP／UDP接続・待受の使用形式を表示する． |
| [`main()`](../userland/apps/nc.c#L45) | 45 | 外部 | TCP・UDPソケットを開き，ネットワークと標準入出力の間でデータを転送する． |

### `userland/apps/net.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`show()`](../userland/apps/net.c#L8) | 8 | static | ネットワーク診断用の/procファイルを開き，内容を出力する． |
| [`main()`](../userland/apps/net.c#L28) | 28 | 外部 | Wi-Fi・ARP等のネットワーク状態と診断情報を表示する． |

### `userland/apps/omikuji.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`random_from_device()`](../userland/apps/omikuji.c#L12) | 12 | static | /dev/randから32ビットの値を読む． |
| [`fortune()`](../userland/apps/omikuji.c#L22) | 22 | static | 乱数値をおみくじの結果文字列へ対応付ける． |
| [`main()`](../userland/apps/omikuji.c#L36) | 36 | 外部 | 擬似乱数で運勢を選び，結果を表示する． |

### `userland/apps/ping.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`ipv4()`](../userland/apps/ping.c#L11) | 11 | static | ドット区切りのIPv4アドレスを4バイトへ変換する． |
| [`main()`](../userland/apps/ping.c#L25) | 25 | 外部 | ICMPデバイスを使って指定IPv4宛てにEchoを送信し，応答とタイムアウトを表示する． |

### `userland/apps/pins.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`describe()`](../userland/apps/pins.c#L22) | 22 | static | GPIOの割当状態から使用用途の表示文字列を求める． |
| [`pinlabel()`](../userland/apps/pins.c#L38) | 38 | static | GPIO番号と現在の使用モードを組み合わせ，基板図に表示するピン名を作る． |
| [`main()`](../userland/apps/pins.c#L44) | 44 | 外部 | ボードのGPIO割当と予約状態を一覧表示する． |

### `userland/apps/pong.c`（6件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`sendall()`](../userland/apps/pong.c#L16) | 16 | static | 端末FDへの表示データを可能な限り書き切る． |
| [`message()`](../userland/apps/pong.c#L28) | 28 | static | 同じ案内文字列を両方の端末FDへ送る． |
| [`row()`](../userland/apps/pong.c#L34) | 34 | static | ゲーム盤の指定行に障害物・ラケット・ボールを描画する． |
| [`render()`](../userland/apps/pong.c#L41) | 41 | static | ゲーム画面全体を端末へ描画する． |
| [`pong_key()`](../userland/apps/pong.c#L79) | 79 | static | 端末ごとにANSI矢印キーを解析する． |
| [`main()`](../userland/apps/pong.c#L98) | 98 | 外部 | 端末上でポンゲームを実行し，入力と描画を周期的に処理する． |

### `userland/apps/ps.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/ps.c#L10) | 10 | 外部 | /proc/tasksからPIDと状態などを取得し，プロセス一覧を表示する． |

### `userland/apps/pwd.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/pwd.c#L7) | 7 | 外部 | 現在の作業ディレクトリを表示する． |

### `userland/apps/pwmctl.c`（7件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`usage()`](../userland/apps/pwmctl.c#L10) | 10 | static | PWM周波数・デューティ比の操作方法を表示する． |
| [`eq()`](../userland/apps/pwmctl.c#L20) | 20 | static | サブコマンド文字列の一致を確認する． |
| [`read_value()`](../userland/apps/pwmctl.c#L23) | 23 | static | PWMデバイスファイルを読み，終端改行を除いた文字列を返す． |
| [`set_value()`](../userland/apps/pwmctl.c#L34) | 34 | static | PWMデバイスファイルに数値文字列を書き込み，成功時に0を返す． |
| [`valid_value()`](../userland/apps/pwmctl.c#L44) | 44 | static | 周波数またはデューティ比を桁数・範囲・小数形式で検証する． |
| [`status()`](../userland/apps/pwmctl.c#L62) | 62 | static | PWMの周波数と各チャネルのデューティ比をデバイスから読み表示する． |
| [`main()`](../userland/apps/pwmctl.c#L74) | 74 | 外部 | /dev/pwm/1で周波数・デューティ比の照会と変更を行う． |

### `userland/apps/python.c`（47件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`outn()`](../userland/apps/python.c#L16) | 16 | static | 標準出力へ指定長を書き切る． |
| [`outs()`](../userland/apps/python.c#L22) | 22 | static | NUL終端文字列を標準出力へ書き込む． |
| [`same()`](../userland/apps/python.c#L25) | 25 | static | 二つの識別子やキーワードを文字列として比較する． |
| [`idchar()`](../userland/apps/python.c#L28) | 28 | static | 文字が英数字または下線なら真を返す． |
| [`items()`](../userland/apps/python.c#L45) | 45 | static | 固定長プール内の指定配列が格納される先頭ポインタを得る． |
| [`string_at()`](../userland/apps/python.c#L48) | 48 | static | 固定長プール内の指定文字列が格納される先頭ポインタを得る． |
| [`seqslot()`](../userland/apps/python.c#L51) | 51 | static | 変数名に対応する既存の固定スロット番号を探す． |
| [`slot_for()`](../userland/apps/python.c#L54) | 54 | static | 変数名のスロットを検索・確保する． |
| [`replace_data()`](../userland/apps/python.c#L64) | 64 | static | 配列・文字列を固定長プール内で置き換える． |
| [`seqnew()`](../userland/apps/python.c#L91) | 91 | static | 固定長プールに配列・タプル相当の要素列を格納し，変数へ関連付ける． |
| [`setstring()`](../userland/apps/python.c#L100) | 100 | static | 最大64文字の文字列を固定長プールに格納する． |
| [`transfer()`](../userland/apps/python.c#L112) | 112 | static | プール内のオブジェクトを詰め直す． |
| [`ws()`](../userland/apps/python.c#L127) | 127 | static | 解析カーソルを空白・タブの後まで進める． |
| [`eat()`](../userland/apps/python.c#L130) | 130 | static | 空白を飛ばした位置で指定1文字を消費する． |
| [`word()`](../userland/apps/python.c#L133) | 133 | static | キーワードを単語境界も含めて照合し，成功時だけカーソルを進める． |
| [`getvar()`](../userland/apps/python.c#L136) | 136 | static | 名前で整数変数を検索する． |
| [`setvar()`](../userland/apps/python.c#L139) | 139 | static | 整数変数へ代入し，旧配列等があればプール領域を解放する． |
| [`divide()`](../userland/apps/python.c#L143) | 143 | static | 符号付き32ビットの整数除算を行い，ゼロ除算と最小値の境界を明示的に扱う． |
| [`parse_string()`](../userland/apps/python.c#L155) | 155 | static | 文字列リテラルを読み，エスケープと領域上限を検査する． |
| [`payload()`](../userland/apps/python.c#L184) | 184 | static | ソケットの送信データは文字列・バイト列・str(整数)から作れる． |
| [`parse_ip()`](../userland/apps/python.c#L215) | 215 | static | IPv4のドット区切り表記をncと共通のlwIPアドレス表現へ変換する． |
| [`socket_address()`](../userland/apps/python.c#L232) | 232 | static | 通信APIに使うIPv4アドレスとポートを解析する． |
| [`valid_port()`](../userland/apps/python.c#L248) | 248 | static | 通信先のポート番号が扱える範囲か検査する． |
| [`parse_fd()`](../userland/apps/python.c#L251) | 251 | static | 引数で受け取ったFD番号を検証・変換する． |
| [`read_line()`](../userland/apps/python.c#L254) | 254 | static | UART/USBのバイト列から長さ制限のある一行を組み立てる． |
| [`input_text()`](../userland/apps/python.c#L271) | 271 | static | input()の文字列を固定領域へ取得する． |
| [`parse_decimal()`](../userland/apps/python.c#L292) | 292 | static | 十進文字列を符号付き32ビット整数へ変換する． |
| [`decimal_string()`](../userland/apps/python.c#L307) | 307 | static | 32ビット整数を表示用の10進文字列へ変換する． |
| [`integer_conversion()`](../userland/apps/python.c#L315) | 315 | static | int()で整数式，文字列変数，input()の結果を整数化する． |
| [`atom()`](../userland/apps/python.c#L339) | 339 | static | 整数式の最小単位を解析する． |
| [`product()`](../userland/apps/python.c#L459) | 459 | static | 乗除算の優先順位を処理する． |
| [`sum()`](../userland/apps/python.c#L465) | 465 | static | 加減算を含む式を評価し，次の構文解析位置を更新する． |
| [`bits()`](../userland/apps/python.c#L469) | 469 | static | 再帰下降で演算子の優先順位を扱う． |
| [`comparison()`](../userland/apps/python.c#L493) | 493 | static | 比較演算を解析する． |
| [`logical_not()`](../userland/apps/python.c#L511) | 511 | static | 比較演算よりnot，さらにand，orの順に優先順位が下がる． |
| [`logical_and()`](../userland/apps/python.c#L514) | 514 | static | 論理AND演算を含む式を解析・評価する． |
| [`expr()`](../userland/apps/python.c#L522) | 522 | static | 演算子の優先順位に従って式を評価する入口． |
| [`number()`](../userland/apps/python.c#L530) | 530 | static | 数値リテラルを解析し，整数値を返す． |
| [`print_value()`](../userland/apps/python.c#L535) | 535 | static | 値を出力する． |
| [`statement()`](../userland/apps/python.c#L574) | 574 | static | 一文の構文と処理を実行する． |
| [`block_end()`](../userland/apps/python.c#L709) | 709 | static | インデントに従って現在の複合文の終端行を探す． |
| [`execute()`](../userland/apps/python.c#L713) | 713 | static | インデントで表される文ブロックを実行し，ループ制御を呼出し元へ伝える． |
| [`join_continuations()`](../userland/apps/python.c#L788) | 788 | static | リストやbytes等の未閉鎖の括弧に含まれる複数行を，インデントを維持して結合する． |
| [`run_source()`](../userland/apps/python.c#L805) | 805 | static | 与えられたソースの先頭から文を順に実行する． |
| [`pending_brackets()`](../userland/apps/python.c#L828) | 828 | static | REPL入力に未閉鎖の括弧が残っているか調べる． |
| [`repl()`](../userland/apps/python.c#L841) | 841 | static | REPL入力を読み，継続行を処理して一文ずつ実行する． |
| [`main()`](../userland/apps/python.c#L882) | 882 | 外部 | ファイル実行と対話実行を引数に応じて切り替える． |

### `userland/apps/reset.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/reset.c#L9) | 9 | 外部 | TTYのカノニカル入力・エコーとANSI表示を復元する． |

### `userland/apps/rm.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/rm.c#L7) | 7 | 外部 | 指定したRAMファイルをVFSのunlinkで削除する． |

### `userland/apps/shell.c`（25件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`eq()`](../userland/apps/shell.c#L14) | 14 | static | 二つの文字列を完全一致で比較する． |
| [`prefix()`](../userland/apps/shell.c#L17) | 17 | static | 指定した接頭辞で文字列が始まるか判定する． |
| [`write_all()`](../userland/apps/shell.c#L20) | 20 | static | カーネルのTXキューが満杯なら書込みが途中で終わる． |
| [`put()`](../userland/apps/shell.c#L30) | 30 | static | 標準出力へNUL終端文字列を表示する． |
| [`resolve()`](../userland/apps/shell.c#L40) | 40 | static | 作業ディレクトリ基準でパスを解決する． |
| [`trim()`](../userland/apps/shell.c#L71) | 71 | static | リダイレクト先の名前に空白を含めず，周囲の空白を構文として扱う． |
| [`restore_fd()`](../userland/apps/shell.c#L79) | 79 | static | 一時的に変更した標準FDを復元し，退避用FDを閉じる． |
| [`restore_stdio()`](../userland/apps/shell.c#L87) | 87 | static | シェルで一時変更した標準入出力FDを元の設定へ戻す． |
| [`redirect()`](../userland/apps/shell.c#L93) | 93 | static | ファイルまたはデバイスへ標準FDを付け替える． |
| [`run_external()`](../userland/apps/shell.c#L141) | 141 | static | 登録済み外部コマンドをIDで起動する． |
| [`pipeline_spawn()`](../userland/apps/shell.c#L187) | 187 | static | パイプの一段を起動し，標準FDの0〜2だけを継承して入出力と前景状態を設定する． |
| [`run_pipeline()`](../userland/apps/shell.c#L216) | 216 | static | 最大3段のパイプを組み立てる． |
| [`command()`](../userland/apps/shell.c#L255) | 255 | static | 入力行を解釈し，組込みコマンド／外部コマンドへ振り分ける． |
| [`history_add()`](../userland/apps/shell.c#L292) | 292 | static | 固定長のコマンド履歴へ追加する． |
| [`shell_move_left()`](../userland/apps/shell.c#L301) | 301 | static | 現在位置からn桁だけ左へ移動する． |
| [`shell_columns()`](../userland/apps/shell.c#L311) | 311 | static | プロンプトも含め，端末のTAB位置は4桁刻みとする． |
| [`shell_has_tab()`](../userland/apps/shell.c#L318) | 318 | static | シェル入力にタブが含まれるか調べ，補完処理の分岐に使う． |
| [`shell_print_expanded()`](../userland/apps/shell.c#L324) | 324 | static | 補完候補や展開後の入力行を端末に表示する． |
| [`shell_redraw()`](../userland/apps/shell.c#L336) | 336 | static | TABを4桁位置へ展開して一行を再描画する． |
| [`shell_redraw_tail()`](../userland/apps/shell.c#L344) | 344 | static | 入力行の変更部分だけ再描画し，プロンプトと前半はそのままにする． |
| [`shell_complete()`](../userland/apps/shell.c#L351) | 351 | static | 行末のファイル名または/binコマンドを補完する． |
| [`shell_list_matches()`](../userland/apps/shell.c#L408) | 408 | static | 候補を画面に表示して入力行を再描画する． |
| [`shell_readline()`](../userland/apps/shell.c#L445) | 445 | static | 一行入力中だけTTYをraw／エコーなしへ切り替え，編集後に元へ戻す． |
| [`run_boot_rc()`](../userland/apps/shell.c#L575) | 575 | static | 最初の起動シェルで設定ファイルの各行を通常のコマンド解析器へ渡す． |
| [`main()`](../userland/apps/shell.c#L599) | 599 | 外部 | 端末の用意と起動時処理を終えた後，入力・解析・実行・子の回収を繰り返す． |

### `userland/apps/stty.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`set()`](../userland/apps/stty.c#L8) | 8 | static | TTYのカノニカル・エコー等の設定をioctlで変更する． |
| [`main()`](../userland/apps/stty.c#L11) | 11 | 外部 | TTY設定を照会・切替する． |

### `userland/apps/tcp_echod.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`port()`](../userland/apps/tcp_echod.c#L7) | 7 | static | 待受ポートの数値指定を検査する． |
| [`main()`](../userland/apps/tcp_echod.c#L15) | 15 | 外部 | 指定TCPポートで接続を待ち，受信データを送り返す． |

### `userland/apps/tcpdump.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`ip()`](../userland/apps/tcpdump.c#L11) | 11 | static | IPv4アドレス4バイトをドット区切りで表示する． |
| [`protocol()`](../userland/apps/tcpdump.c#L14) | 14 | static | IPv4プロトコル番号をTCP／UDP／ICMP等の表示名に対応付ける． |
| [`show()`](../userland/apps/tcpdump.c#L19) | 19 | static | 保存されたパケット要約を読める形式で出力する． |
| [`main()`](../userland/apps/tcpdump.c#L47) | 47 | 外部 | /dev/net/packetから受信・送信パケットの要約を読み，指定件数まで表示する． |

### `userland/apps/trace.c`（21件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`syscall_name()`](../userland/apps/trace.c#L13) | 13 | static | トレース中のシステムコール番号を表示名へ変換する． |
| [`load_program_names()`](../userland/apps/trace.c#L29) | 29 | static | 登録プログラムのIDと名前をVFSから読み，記録表示に使う． |
| [`program_name()`](../userland/apps/trace.c#L50) | 50 | static | プログラムIDから登録名を取得する． |
| [`find_fd()`](../userland/apps/trace.c#L59) | 59 | static | 追跡対象プロセスのFDに付けた表示名を探す． |
| [`forget_fd()`](../userland/apps/trace.c#L67) | 67 | static | 終了・closeされたFDの表示名キャッシュを削除する． |
| [`save_fd()`](../userland/apps/trace.c#L74) | 74 | static | openしたFDと表示パスの組を小さな履歴に保存する． |
| [`col()`](../userland/apps/trace.c#L84) | 84 | static | 列幅を固定してトレース表示を小さく予測しやすく保つ． |
| [`numeric()`](../userland/apps/trace.c#L92) | 92 | static | 記録内の数値フィールドを表示用に書式化する． |
| [`format_short_time()`](../userland/apps/trace.c#L95) | 95 | static | トレース時刻を短い固定桁の表示へ変換する． |
| [`append()`](../userland/apps/trace.c#L112) | 112 | static | 文字列バッファ末尾に表示項目を追加し，上限を越えないようにする． |
| [`device_path()`](../userland/apps/trace.c#L119) | 119 | static | デバイスIDの名称はカーネルVFSと共通の定義から求める． |
| [`format_target()`](../userland/apps/trace.c#L128) | 128 | static | システムコールのFD／対象を人間が読みやすい短い文字列へ整形する． |
| [`print_preview()`](../userland/apps/trace.c#L137) | 137 | static | read／writeの先頭データを印字可能な範囲で表示する． |
| [`header()`](../userland/apps/trace.c#L154) | 154 | static | トレース表示の列名と区切り線を出力する． |
| [`pending_find()`](../userland/apps/trace.c#L162) | 162 | static | 入口イベントに対応する未完了システムコールを探す． |
| [`pending_save()`](../userland/apps/trace.c#L170) | 170 | static | 入口イベントを保存し，後続の終了イベントと対応付ける． |
| [`reset_view_state()`](../userland/apps/trace.c#L177) | 177 | static | 対象PIDやFD履歴等のトレース表示状態を初期化する． |
| [`print_event()`](../userland/apps/trace.c#L187) | 187 | static | リングから取得した1イベントを表示する． |
| [`trace_drain()`](../userland/apps/trace.c#L244) | 244 | static | トレースデバイスから固定長レコードを読み，表示へ順次流す． |
| [`trace_run()`](../userland/apps/trace.c#L252) | 252 | static | 指定コマンドをトレースし，待機と表示処理を行う． |
| [`main()`](../userland/apps/trace.c#L320) | 320 | 外部 | トレース表示・起動操作をコマンド引数に基づいて選ぶ． |

### `userland/apps/tree.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`traverse()`](../userland/apps/tree.c#L10) | 10 | static | 指定パスを階層的に探索し，深さ上限を守って子をたどる． |
| [`visit()`](../userland/apps/tree.c#L18) | 18 | static | ディレクトリエントリを木構造の字下げ付きで表示する． |
| [`main()`](../userland/apps/tree.c#L27) | 27 | 外部 | VFSディレクトリを再帰的にたどり，階層をツリー表示する． |

### `userland/apps/tty.c`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`main()`](../userland/apps/tty.c#L7) | 7 | 外部 | 現在タスクが接続されているTTYの情報を表示する． |

### `userland/apps/udp_echod.c`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`port()`](../userland/apps/udp_echod.c#L7) | 7 | static | UDP待受ポート番号を解析する． |
| [`main()`](../userland/apps/udp_echod.c#L15) | 15 | 外部 | UDP受信データを送信元へ送り返すエコーサーバを動かす． |

### `userland/apps/vi.c`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`poll_key()`](../userland/apps/vi.c#L12) | 12 | static | ANSIカーソルキーの残りを読み出し，単独Escをブロックしない． |
| [`move_arrow()`](../userland/apps/vi.c#L26) | 26 | static | エスケープシーケンスによるカーソル移動指示を処理する． |
| [`main()`](../userland/apps/vi.c#L35) | 35 | 外部 | 共通の編集処理を用い，vi風キー操作でRAMファイルを編集する． |

### `userland/apps/walk.h`（5件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`walk_eq()`](../userland/apps/walk.h#L14) | 14 | static | 二つのパス構成要素を一致比較する． |
| [`walk_normalize()`](../userland/apps/walk.h#L17) | 17 | static | 作業ディレクトリと相対パスを解決し，.と..を含む経路を正規化する． |
| [`walk_dir()`](../userland/apps/walk.h#L44) | 44 | static | 指定したパスがgetdentsで列挙できるディレクトリか判定する． |
| [`walk_entries()`](../userland/apps/walk.h#L51) | 51 | static | ディレクトリを列挙してコールバックへ名前と種別を渡す． |
| [`walk_join()`](../userland/apps/walk.h#L62) | 62 | static | 親ディレクトリと子の名前を結合する． |

### `userland/apps/watch.c`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`capture_byte()`](../userland/apps/watch.c#L23) | 23 | static | 子プロセスの出力1バイトを画面スナップショットへ反映する． |
| [`redraw()`](../userland/apps/watch.c#L86) | 86 | static | 対象コマンドの出力を再描画し，画面を更新する． |
| [`capture_command()`](../userland/apps/watch.c#L97) | 97 | static | 256バイトのカーネルパイプは子が動いている間に排出する． |
| [`main()`](../userland/apps/watch.c#L125) | 125 | 外部 | 指定コマンドを一定間隔で再実行し，結果を端末へ表示する． |

### `userland/lib/libc.c`（59件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`strlen()`](../userland/lib/libc.c#L28) | 28 | 外部 | NUL終端文字列の長さを数える． |
| [`strcpy()`](../userland/lib/libc.c#L31) | 31 | 外部 | 文字列を終端文字まで宛先へコピーし，宛先を返す． |
| [`strncpy()`](../userland/lib/libc.c#L34) | 34 | 外部 | 最大nバイトをコピーする． |
| [`strcat()`](../userland/lib/libc.c#L37) | 37 | 外部 | 宛先文字列の末尾に別の文字列を追加する． |
| [`strcmp()`](../userland/lib/libc.c#L40) | 40 | 外部 | 二つの文字列を辞書順で比較し，差の符号を返す． |
| [`strncmp()`](../userland/lib/libc.c#L43) | 43 | 外部 | 文字列を最大n文字比較し，最初に異なるバイトの差を返す． |
| [`strchr()`](../userland/lib/libc.c#L46) | 46 | 外部 | 文字列から指定文字を探し，最初の位置またはNULLを返す． |
| [`strstr()`](../userland/lib/libc.c#L49) | 49 | 外部 | 文字列から部分文字列を探し，最初の位置またはNULLを返す． |
| [`memcpy()`](../userland/lib/libc.c#L52) | 52 | 外部 | バイト単位の処理にして，コンパイラが外部libc呼出しを生成しないようにする． |
| [`memmove()`](../userland/lib/libc.c#L55) | 55 | 外部 | 領域が重なる場合も正しい順序でnバイト移動する． |
| [`memset()`](../userland/lib/libc.c#L58) | 58 | 外部 | 指定領域のnバイトを同じ値で埋める． |
| [`__aeabi_memcpy()`](../userland/lib/libc.c#L61) | 61 | 外部 | Clang -Ozは構造体コピーなどをARM EABIのメモリ操作へ変換することがある． |
| [`__aeabi_memcpy4()`](../userland/lib/libc.c#L64) | 64 | 外部 | ARM EABIの4バイト整列コピー要求を自前のmemcpyへ委譲する． |
| [`__aeabi_memcpy8()`](../userland/lib/libc.c#L67) | 67 | 外部 | ARM EABIの8バイト整列コピー要求を自前のmemcpyへ委譲する． |
| [`__aeabi_memmove()`](../userland/lib/libc.c#L70) | 70 | 外部 | ARM EABIの重複可能なメモリ移動を自前のmemmoveへ委譲する． |
| [`__aeabi_memset()`](../userland/lib/libc.c#L73) | 73 | 外部 | EABIのmemsetはISO Cのmemsetと異なり，引数順が宛先・長さ・値である． |
| [`__aeabi_memset4()`](../userland/lib/libc.c#L76) | 76 | 外部 | ARM EABIの4バイト整列初期化を自前のmemsetへ委譲する． |
| [`__aeabi_memset8()`](../userland/lib/libc.c#L79) | 79 | 外部 | ARM EABIの8バイト整列初期化を自前のmemsetへ委譲する． |
| [`memchr()`](../userland/lib/libc.c#L82) | 82 | 外部 | 最大nバイトを検索し，最初に指定値と一致した位置を返す． |
| [`memcmp()`](../userland/lib/libc.c#L89) | 89 | 外部 | 指定長の二つのメモリ領域を比較し，最初の不一致の差を返す． |
| [`abs()`](../userland/lib/libc.c#L92) | 92 | 外部 | 整数の絶対値を返す． |
| [`strtol()`](../userland/lib/libc.c#L94) | 94 | 外部 | ISO Cと同様にabs(INT_MIN)の動作は未定義である． |
| [`strtoul()`](../userland/lib/libc.c#L119) | 119 | 外部 | 符号と基数接頭辞を解釈して整数へ変換する． |
| [`atoi()`](../userland/lib/libc.c#L149) | 149 | 外部 | 文字列の先頭を符号付き10進整数として読み取る． |
| [`output()`](../userland/lib/libc.c#L152) | 152 | static | 標準出力のバッファを部分書込みまで処理して送る． |
| [`fflush()`](../userland/lib/libc.c#L165) | 165 | 外部 | 保留中の文字を送信する． |
| [`setvbuf()`](../userland/lib/libc.c#L188) | 188 | 外部 | FILEストリームのバッファリング方式を設定する． |
| [`stream_put()`](../userland/lib/libc.c#L199) | 199 | static | FILEの出力条件を検査して1文字送る． |
| [`stream_get()`](../userland/lib/libc.c#L209) | 209 | static | FILEの読出し位置とungetc用保存バイトを管理する． |
| [`stream_unget()`](../userland/lib/libc.c#L223) | 223 | static | 直前に読んだ1文字をFILEへ戻し，次回読出しへ備える． |
| [`feof()`](../userland/lib/libc.c#L230) | 230 | 外部 | FILEストリームの終端到達フラグを返す． |
| [`ferror()`](../userland/lib/libc.c#L233) | 233 | 外部 | FILEストリームの入出力エラーフラグを返す． |
| [`fopen()`](../userland/lib/libc.c#L236) | 236 | 外部 | openしたFDを小さなFILE管理表に関連付ける． |
| [`fclose()`](../userland/lib/libc.c#L254) | 254 | 外部 | 保留中の出力を流し，FDとFILE管理枠を解放する． |
| [`fread()`](../userland/lib/libc.c#L262) | 262 | 外部 | 指定サイズ・個数で入力を読み取る． |
| [`fwrite()`](../userland/lib/libc.c#L271) | 271 | 外部 | 指定サイズ・個数の出力を行う． |
| [`putchar()`](../userland/lib/libc.c#L280) | 280 | 外部 | stdoutへ1文字出力する． |
| [`getchar()`](../userland/lib/libc.c#L283) | 283 | 外部 | stdinから1文字取得する． |
| [`puts()`](../userland/lib/libc.c#L286) | 286 | 外部 | 文字列をstdoutへ出力し，末尾に改行を付ける． |
| [`fgets()`](../userland/lib/libc.c#L292) | 292 | 外部 | FILEから最大size-1文字を読み，改行またはEOFまでをNUL終端で格納する． |
| [`emit()`](../userland/lib/libc.c#L302) | 302 | static | 書式出力の1文字を，ファイルまたは文字列バッファの出力先へ渡す． |
| [`digits()`](../userland/lib/libc.c#L309) | 309 | static | 指定基数で整数を桁の文字列へ変換する． |
| [`pad()`](../userland/lib/libc.c#L316) | 316 | static | 書式指定の最小幅に合わせて空白等を出力する． |
| [`format()`](../userland/lib/libc.c#L319) | 319 | static | 限定的なprintf書式指定を解釈して文字を出力する． |
| [`printf()`](../userland/lib/libc.c#L371) | 371 | 外部 | 簡易書式解析でstdoutへ整形出力する． |
| [`fprintf()`](../userland/lib/libc.c#L374) | 374 | 外部 | 簡易書式解析で指定FILEへ整形出力する． |
| [`snprintf()`](../userland/lib/libc.c#L377) | 377 | 外部 | 最大サイズを守って整形文字列を生成する． |
| [`sprintf()`](../userland/lib/libc.c#L380) | 380 | 外部 | サイズ指定のない書式出力を行う． |
| [`scan_get()`](../userland/lib/libc.c#L385) | 385 | static | 書式入力処理で次の1文字を取り出す． |
| [`scan_unget()`](../userland/lib/libc.c#L398) | 398 | static | 書式入力処理で読んだ1文字を入力元へ戻す． |
| [`scan_space()`](../userland/lib/libc.c#L406) | 406 | static | 書式入力で先頭の空白を読み飛ばす． |
| [`scan_digit()`](../userland/lib/libc.c#L409) | 409 | static | 書式入力で1桁の数字を指定基数として解釈する． |
| [`scan_run()`](../userland/lib/libc.c#L417) | 417 | static | 限定的なscanf書式を解析する． |
| [`scanf()`](../userland/lib/libc.c#L513) | 513 | 外部 | stdinから簡易書式に従って値を読み取る． |
| [`fscanf()`](../userland/lib/libc.c#L519) | 519 | 外部 | 指定FILEから簡易書式に従って値を読み取る． |
| [`sscanf()`](../userland/lib/libc.c#L526) | 526 | 外部 | 入力文字列から簡易書式に従って値を読み取る． |
| [`__aeabi_memclr()`](../userland/lib/libc.c#L532) | 532 | 外部 | 一部のコンパイラがローカル構造体のゼロ初期化で生成するARM EABI補助関数． |
| [`__aeabi_memclr4()`](../userland/lib/libc.c#L538) | 538 | 外部 | ARM EABIの4バイト整列ゼロクリア要求をmemsetへ委譲する． |
| [`__aeabi_memclr8()`](../userland/lib/libc.c#L541) | 541 | 外部 | ARM EABIの8バイト整列ゼロクリア要求をmemsetへ委譲する． |

### `userland/lib/screen.h`（4件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`screen_enter()`](../userland/lib/screen.h#L13) | 13 | static | 全画面操作用に代替画面へ切り替え，カーソルを隠す． |
| [`screen_leave()`](../userland/lib/screen.h#L16) | 16 | static | 全画面表示を終え，元の端末画面とカーソルを復元する． |
| [`screen_move()`](../userland/lib/screen.h#L19) | 19 | static | ANSIエスケープシーケンスで指定行・列へカーソルを移す． |
| [`screen_measure()`](../userland/lib/screen.h#L22) | 22 | static | 端末サイズを問い合わせ，取得できなければ指定された既定値を利用する． |

### `userland/lib/syscalls.c`（49件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`call()`](../userland/lib/syscalls.c#L8) | 8 | static | r0にシステムコール番号n，r1〜r3に引数a〜cを置いてSVCを発行する． |
| [`chdir()`](../userland/lib/syscalls.c#L18) | 18 | 外部 | SVCを発行して現在の作業ディレクトリを変更する． |
| [`getcwd()`](../userland/lib/syscalls.c#L21) | 21 | 外部 | SVCから現在ディレクトリのパスを受け取る． |
| [`getpid()`](../userland/lib/syscalls.c#L24) | 24 | 外部 | SVCから現在タスクのPIDを取得する． |
| [`flash_id()`](../userland/lib/syscalls.c#L27) | 27 | 外部 | SVCからFlash固有IDの下位32ビットを取得する． |
| [`kill()`](../userland/lib/syscalls.c#L30) | 30 | 外部 | SVCで指定PIDのタスクに終了を要求する． |
| [`_exit()`](../userland/lib/syscalls.c#L33) | 33 | 外部 | SVCで現在タスクを終了する． |
| [`msleep()`](../userland/lib/syscalls.c#L36) | 36 | 外部 | SVCで指定ミリ秒だけタスクを睡眠させる． |
| [`yield()`](../userland/lib/syscalls.c#L39) | 39 | 外部 | SVCで実行権をほかのREADYタスクへ譲る． |
| [`open()`](../userland/lib/syscalls.c#L42) | 42 | 外部 | SVCでVFS上のパスを開き，FDまたはエラーを返す． |
| [`close()`](../userland/lib/syscalls.c#L45) | 45 | 外部 | SVCでFDを閉じる． |
| [`lseek()`](../userland/lib/syscalls.c#L48) | 48 | 外部 | SVCでFDのファイル位置を変更する． |
| [`read()`](../userland/lib/syscalls.c#L51) | 51 | 外部 | SVCで指定FDから最大nバイトを読み込む． |
| [`write()`](../userland/lib/syscalls.c#L54) | 54 | 外部 | SVCで指定FDへ最大nバイトを書き込む． |
| [`ioctl()`](../userland/lib/syscalls.c#L57) | 57 | 外部 | SVCでTTYやデバイスの固有制御を要求する． |
| [`fcntl()`](../userland/lib/syscalls.c#L60) | 60 | 外部 | SVCでFDのフラグ・非ブロック設定等を操作する． |
| [`waitpid()`](../userland/lib/syscalls.c#L63) | 63 | 外部 | SVCで指定タスクの終了を待ち，必要なら終了状態を受け取る． |
| [`getdents()`](../userland/lib/syscalls.c#L66) | 66 | 外部 | SVCでディレクトリエントリを一件ずつ読み取る． |
| [`unlink()`](../userland/lib/syscalls.c#L69) | 69 | 外部 | SVCでVFS上のRAMファイルを削除する． |
| [`pipe()`](../userland/lib/syscalls.c#L72) | 72 | 外部 | SVCでパイプの読取FD・書込FDを配列へ返す． |
| [`dup()`](../userland/lib/syscalls.c#L75) | 75 | 外部 | SVCで既存FDを未使用番号へ複製する． |
| [`dup2()`](../userland/lib/syscalls.c#L78) | 78 | 外部 | SVCで既存FDを指定FD番号へ複製する． |
| [`program_id()`](../userland/lib/syscalls.c#L81) | 81 | 外部 | 名前に対応する登録済み実行プログラムのIDを取得する． |
| [`spawn()`](../userland/lib/syscalls.c#L84) | 84 | 外部 | プログラムIDと引数で子タスクを起動し，PIDを返す． |
| [`spawn_foreground()`](../userland/lib/syscalls.c#L87) | 87 | 外部 | 子タスクを前景指定付きで起動する． |
| [`tty_spawn()`](../userland/lib/syscalls.c#L90) | 90 | 外部 | 指定TTY向けに登録プログラムを起動する． |
| [`set_foreground()`](../userland/lib/syscalls.c#L93) | 93 | 外部 | TTYの前景プロセスを指定PIDへ切り替える． |
| [`console_ready()`](../userland/lib/syscalls.c#L96) | 96 | 外部 | コンソールが利用可能な状態かSVCで問い合わせる． |
| [`get_tty()`](../userland/lib/syscalls.c#L99) | 99 | 外部 | 現在のタスクが所属するTTY番号を取得する． |
| [`spawn_tmp()`](../userland/lib/syscalls.c#L102) | 102 | 外部 | /tmpの実行イメージを読み出し，子タスクとして起動する． |
| [`spawn_tmp_foreground()`](../userland/lib/syscalls.c#L105) | 105 | 外部 | /tmpの実行イメージをTTY前景タスクとして起動する． |
| [`socket()`](../userland/lib/syscalls.c#L108) | 108 | 外部 | ドメイン・種別・プロトコルを検査するカーネルへソケット作成を要求する． |
| [`bind_udp()`](../userland/lib/syscalls.c#L111) | 111 | 外部 | UDPソケットを指定ポートへ束縛する． |
| [`sendto_udp()`](../userland/lib/syscalls.c#L114) | 114 | 外部 | 宛先情報を含むPicoNIX独自UDP構造体で送信する． |
| [`recvfrom_udp()`](../userland/lib/syscalls.c#L117) | 117 | 外部 | PicoNIX独自UDP構造体へ送信元と受信内容を取得する． |
| [`tcp_listen()`](../userland/lib/syscalls.c#L120) | 120 | 外部 | システムコール番号はkernel/kernel.cと共通の定義を使う． |
| [`tcp_connect()`](../userland/lib/syscalls.c#L123) | 123 | 外部 | 接続先構造体を渡し，TCP接続を非同期に開始する． |
| [`tcp_status()`](../userland/lib/syscalls.c#L126) | 126 | 外部 | TCPソケットの接続状態をカーネルへ問い合わせる． |
| [`tcp_accept()`](../userland/lib/syscalls.c#L129) | 129 | 外部 | TCP待受FDから確立済み接続を取り出す． |
| [`ipv4_addr()`](../userland/lib/syscalls.c#L132) | 132 | static | BSD風ソケットのアドレスをIPv4に限定して検査する． |
| [`bind()`](../userland/lib/syscalls.c#L140) | 140 | 外部 | bindのIPv4アドレスとポートを検証し，PicoNIXのUDP bindまたはTCP listenの呼出しに変換する． |
| [`listen()`](../userland/lib/syscalls.c#L153) | 153 | 外部 | PicoNIXではTCPのbind時点で待受を開始するため，ここでは引数の簡易検証のみ行う． |
| [`accept()`](../userland/lib/syscalls.c#L159) | 159 | 外部 | 未対応の接続元アドレス取得を拒否して，TCP接続FDを受け取る． |
| [`connect()`](../userland/lib/syscalls.c#L165) | 165 | 外部 | IPv4のsockaddr_inをPicoNIXのエンドポイントへ変換してTCP接続する． |
| [`shutdown()`](../userland/lib/syscalls.c#L174) | 174 | 外部 | 送信側だけの半閉鎖（SHUT_WR）をサポートする． |
| [`send()`](../userland/lib/syscalls.c#L180) | 180 | 外部 | 送信フラグとバッファを検証し，writeへ委譲する． |
| [`recv()`](../userland/lib/syscalls.c#L186) | 186 | 外部 | 受信フラグとバッファを検証し，readへ委譲する． |
| [`sendto()`](../userland/lib/syscalls.c#L192) | 192 | 外部 | IPv4宛先を独自UDP送信構造体へ変換し，データを送信する． |
| [`recvfrom()`](../userland/lib/syscalls.c#L204) | 204 | 外部 | UDPデータを受信し，必要なら送信元をsockaddr_inへ戻す． |

### `userland/lib/unistd.h`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`htons()`](../userland/lib/unistd.h#L45) | 45 | static | 16ビット整数のバイト順を入れ替える． |

## 開発・ホストツール

### `host/demo_udp.py`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`run()`](../host/demo_udp.py#L17) | 17 | モジュール | UDPデバイス制御サービスへ要求を送信し，結果を表示する実演処理． |
| [`main()`](../host/demo_udp.py#L43) | 43 | モジュール | UDP 2000番のGPIO等の遠隔操作を試すコマンド引数を処理する． |

### `host/piconixctl.py`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`request()`](../host/piconixctl.py#L9) | 9 | モジュール | PicoNIXのUDPデバイス制御サービスへコマンドを送り，応答を受け取る． |
| [`main()`](../host/piconixctl.py#L22) | 22 | モジュール | 接続先・操作対象を解析し，PicoNIX向けのデバイス制御要求を実行する． |

### `tools/audit_firmware.py`（7件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`run()`](../tools/audit_firmware.py#L15) | 15 | モジュール | 外部コマンドを実行し，標準出力を文字列で取得する． |
| [`parse_nm()`](../tools/audit_firmware.py#L19) | 19 | モジュール | nm出力をシンボル名とアドレスの辞書に変換する． |
| [`parse_symbol_sizes()`](../tools/audit_firmware.py#L27) | 27 | モジュール | Parse GNU nm -S output; sizes are from the linked ELF, not constants. |
| [`parse_sections()`](../tools/audit_firmware.py#L39) | 39 | モジュール | objdumpのセクション一覧から名前・サイズ・VMA・LMAを解析する． |
| [`audit()`](../tools/audit_firmware.py#L50) | 50 | モジュール | 実際にリンクしたELFのベクタ・Flash・RAM・ユーザスロット配置を検査する． |
| [`memory_report()`](../tools/audit_firmware.py#L124) | 124 | モジュール | Kernel high-water mark from allocated ELF SRAM sections (includes heap/stack). |
| [`main()`](../tools/audit_firmware.py#L158) | 158 | モジュール | ELF解析ツールを呼び出し，メモリ配置違反があれば失敗扱いにする． |

### `tools/audit_wifi_elf.py`（3件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`parse_sections()`](../tools/audit_wifi_elf.py#L19) | 19 | モジュール | ELFのセクション位置とサイズを読み取り，Wi-Fi構成のRAM配置検査へ渡す． |
| [`audit()`](../tools/audit_wifi_elf.py#L30) | 30 | モジュール | Wi-Fiの予約領域やカーネル境界が重なっていないか確認する． |
| [`main()`](../tools/audit_wifi_elf.py#L45) | 45 | モジュール | Wi-Fi向けELFのメモリ配置検査を起動し，問題を報告する． |

### `tools/gen_functions.py`（10件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`mask_source()`](../tools/gen_functions.py#L27) | 27 | モジュール | コメントと文字列を空白に置換し，行番号と括弧を保つ． |
| [`comment_summary()`](../tools/gen_functions.py#L75) | 75 | モジュール | 直前のコメントから最初の説明文を取り出す． |
| [`preceding_comment()`](../tools/gen_functions.py#L94) | 94 | モジュール | 関数宣言と空白だけを挟んで接するコメントを取得する． |
| [`c_functions()`](../tools/gen_functions.py#L103) | 103 | モジュール | 波括弧の深さ0にあるC関数定義を列挙する．宣言は含めない． |
| [`py_functions()`](../tools/gen_functions.py#L134) | 134 | モジュール | Pythonのトップレベル関数を抽出する（テスト・関数内の補助関数は対象外）． |
| [`asm_functions()`](../tools/gen_functions.py#L153) | 153 | モジュール | .typeで関数と指定されたアセンブリの入口ラベルを抽出する． |
| [`sources()`](../tools/gen_functions.py#L167) | 167 | モジュール | 実装ファイルをパス順に取得する．テストコードは索引を肥大化させるため除外． |
| [`render()`](../tools/gen_functions.py#L179) | 179 | モジュール | 対象ファイルの関数定義と説明を集計し，Markdown形式の関数一覧を構築する． |
| [`anchor()`](../tools/gen_functions.py#L232) | 232 | モジュール | GitHub Markdownにおける日本語見出しのアンカー文字列を返す． |
| [`main()`](../tools/gen_functions.py#L237) | 237 | モジュール | --check指定時は再生成漏れを検出し，通常時は関数一覧ファイルを書き出す． |

### `tools/generate_initial_fs.py`（1件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`generate()`](../tools/generate_initial_fs.py#L11) | 11 | モジュール | 初期RAMファイルの入力データからビルド用のイメージを生成する． |

### `tools/limit_sdk_ram.py`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`transform()`](../tools/limit_sdk_ram.py#L13) | 13 | モジュール | Pico SDKのリンカスクリプトを変換し，カーネルRAM範囲を制限する． |
| [`main()`](../tools/limit_sdk_ram.py#L47) | 47 | モジュール | SDK用リンカスクリプトを読み，変換結果を書き出す． |

### `tools/limit_sdk_xip.py`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`transform()`](../tools/limit_sdk_xip.py#L13) | 13 | モジュール | XIP実行用リンク設定に必要なSDKリンカスクリプトの変更を行う． |
| [`main()`](../tools/limit_sdk_xip.py#L51) | 51 | モジュール | 対象リンカスクリプトを変換し，Flash実行のメモリ配置を整える． |

### `tools/size_ram.py`（17件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`fmt()`](../tools/size_ram.py#L33) | 33 | モジュール | バイト数をKiB単位の桁揃え表示文字列にする． |
| [`command_output()`](../tools/size_ram.py#L37) | 37 | モジュール | 外部ツールを実行して出力を取得し，失敗時は例外を発生させる． |
| [`tool()`](../tools/size_ram.py#L45) | 45 | モジュール | ARM用ツールを優先して探し，見つからなければホスト側コマンドを探す． |
| [`elf_info()`](../tools/size_ram.py#L55) | 55 | モジュール | readelfからELFセクションとシンボル位置を読み取る． |
| [`app_rows()`](../tools/size_ram.py#L71) | 71 | モジュール | 各ユーザイメージのELFを解析し，データ・スタック・空き容量を集計する． |
| [`component()`](../tools/size_ram.py#L95) | 95 | モジュール | Classify *input* object paths; this is not a symbol-level classifier. |
| [`overlap()`](../tools/size_ram.py#L126) | 126 | モジュール | 二つのアドレス区間が重複するか調べる． |
| [`interval_union()`](../tools/size_ram.py#L129) | 129 | モジュール | Count only distinct addressed bytes, avoiding overlap/double counting. |
| [`memory_kind()`](../tools/size_ram.py#L146) | 146 | モジュール | アドレス位置からFlash・SRAM等のメモリ種類を返す． |
| [`allocated_ranges()`](../tools/size_ram.py#L154) | 154 | モジュール | ELFのセクションから実際の割当済みアドレス範囲を集める． |
| [`embedded_ranges()`](../tools/size_ram.py#L158) | 158 | モジュール | Get real embedded PIC byte intervals from the firmware ELF symbol pairs. |
| [`map_report()`](../tools/size_ram.py#L184) | 184 | モジュール | Attribute only bytes that fall inside an ELF allocated output section. |
| [`sram_region_totals()`](../tools/size_ram.py#L249) | 249 | モジュール | Return unique allocated ELF SRAM bytes, partitioned by fixed regions. |
| [`print_fixed_layout()`](../tools/size_ram.py#L270) | 270 | モジュール | PicoNIXの固定RAM領域の開始・終了と容量を表示する． |
| [`kernel_stack_bounds()`](../tools/size_ram.py#L290) | 290 | モジュール | ELFのシンボル表からカーネルスタック領域の境界を調べる． |
| [`print_kernel()`](../tools/size_ram.py#L301) | 301 | モジュール | カーネルのセクション別RAM使用量と上限を表示する． |
| [`main()`](../tools/size_ram.py#L393) | 393 | モジュール | ビルド成果物のRAM割当てとユーザプログラムの使用量を集計表示する． |

### `tools/verify_layout.py`（2件）

| 関数 | 行 | 範囲 | 概要 |
|---|---:|---|---|
| [`symbols()`](../tools/verify_layout.py#L16) | 16 | モジュール | ELFのシンボル名と位置を抽出する． |
| [`main()`](../tools/verify_layout.py#L29) | 29 | モジュール | リンカが出力したイメージ配置を検査し，違反があれば停止する． |
