# ファイルシステムとデバイスノード

[English](../../en/reference/FILESYSTEM.md) | [ドキュメント案内](../../DOCS.ja.md) | [デバイス操作](devices/DEVICES.md)

PicoNIXは小さな仮想ファイルシステム（VFS）を提供します．**ディレクトリやデバイスノードはカーネルが生成し，通常のディスク上には存在しません**．`/tmp`と`/etc`の通常ファイルはSRAM上に保存され，再起動で失われます．この文書は現行の`kernel/kernel.c`のVFS登録・列挙処理と`kernel/pins.h`を基準にしています．

## 1．ディレクトリ構成

```text
/
├── bin/                    組込みユーザプログラム
├── dev/                    デバイス
│   ├── console             主端末
│   ├── ttyS1               UART1端末
│   ├── ttyN0               Wi-Fi端末（Wi-Fiビルドのみ）
│   ├── net/
│   │   └── wifi            STA再接続制御（Wi-Fiビルドのみ）
│   ├── rand                簡易疑似乱数
│   ├── mem                 許可されたメモリ領域の読み書き
│   ├── shm                 2 KiBの共有メモリ
│   ├── trace               バイナリトレース
│   ├── gpio/
│   │   ├── fb              8×8 LEDフレームバッファ
│   │   ├── N               GPIO値
│   │   ├── N.dir           GPIO方向
│   │   └── N.pull          プルアップ／プルダウン設定
│   ├── i2c/
│   │   └── 1               I²C1バス
│   ├── adc/
│   │   ├── 0, 1, 2         ADC入力（設定に依存）
│   │   └── temp            内蔵温度センサー
│   └── pwm/
│       └── 1/              PWMを使用する構成で公開
│           ├── freq        周波数
│           ├── a           GP18のDuty比
│           └── b           GP19のDuty比
├── proc/                   システム状態
│   ├── uptime, ticks, interrupts, context_switches
│   ├── memory, devices, ramfiles, tasks, fb
│   ├── task/PID            実行中タスクごとの状態
│   ├── sym/
│   │   ├── kernel          カーネルシンボル
│   │   └── userland/NAME   登録済みプログラムのシンボル
│   └── net/
│       ├── wifi            Wi-Fi・DHCP・STA/AP状態
│       ├── ap              AP接続端末の情報
│       ├── arp             ARPキャッシュ
│       └── memory          lwIPヒープ・pbuf・アイドルスタック
├── tmp/                    揮発性RAMファイル
└── etc/                    揮発性設定ファイル
    ├── rc                  起動スクリプト
    ├── index.html          Pico Wのトップページ
    ├── fbcheck.html        Pico WのLED画面
    └── pwm.html            Pico WのPWM画面
```

`N`，`PID`，`NAME`はプレースホルダーであり，その文字列のファイルが存在するわけではありません．`/bin`は登録済みの組込みプログラムを列挙します．`/proc/task`には**稼働中のPID**が列挙されます．`/proc/sym/userland`はプログラム名から直接ファイルを開けますが，現在のディレクトリ列挙処理では子要素を一覧表示しません．`/dev/pwm/1`はPWM使用時のみ有効です．`/etc`のHTMLファイルはPico Wの初期構成例であり，通常のPicoには標準では配置されません．`/proc/net/memory`は通常のPicoでも読めますが，lwIPは無効で`MODE: N/A`と`IDLE_STACK`のみを返します．

## 2．ノードの種類とアクセス

| ノード | 読み取り | 書き込み | 備考 |
|---|:---:|:---:|---|
| `/dev/console`, `/dev/ttyS1`, `/dev/ttyN0` | ○ | ○ | 端末．`/dev/ttyN0`はWi-Fiビルドのみ |
| `/dev/rand` | ○ | × | 暗号用途には使えない疑似乱数．1回の読み取りは最大4バイト |
| `/dev/mem` | ○ | 制限付き | XIP Flashと主SRAMの読み取り，ユーザスロット・SHM・RAMファイルデータの書き込み．`lseek()`でアドレス指定 |
| `/dev/shm` | ○ | ○ | 全タスク共通の2 KiB RAM．先頭からの相対オフセットでアクセス |
| `/dev/trace` | ○ | × | バイナリレコード．`trace`コマンドを参照 |
| `/dev/gpio/fb` | ○ | ○ | 8バイト単位．有効化は`ioctl()`で行う |
| `/dev/gpio/N`, `N.dir`, `N.pull` | ○ | ○ | `pins.h`でGPIOに割り当てたピンのみ |
| `/dev/i2c/1` | ○ | ○ | I²C1バス．転送は`ioctl()`で実行．[I²Cガイド](devices/I2C.md)を参照 |
| `/dev/adc/0`〜`2`, `/dev/adc/temp` | ○ | × | 十進テキスト．0〜2はADC割り当て時のみ |
| `/dev/pwm/1/freq`, `a`, `b` | ○ | ○ | PWM割り当てに依存 |
| `/proc`配下の情報ファイル | ○ | × | カーネル状態のスナップショットなど |
| `/tmp/NAME`, `/etc/NAME` | ○ | ○ | RAM上の通常ファイル |
| ディレクトリ | 一覧取得 | × | `getdents()`で列挙．通常ファイルのような`read()`は不可 |

実際の読み書き形式，`ioctl()`の値，ピンの占有・排他条件は[デバイスガイド](devices/DEVICES.md)と[TTY](../literacy/TTY.md)を参照してください．`/proc/devices`は固定デバイスの登録表と`gpio/`，`adc/`，`pwm/`，`i2c/`の入口を列挙するもので，すべての動的ノードを列挙するわけではありません．

## 3．GPIOと周辺機器の公開条件

標準の`kernel/pins.h`では，GP0〜17およびGP22〜25をGPIO，GP18/19をPWM，GP20/21をUART1，GP26/27をI²C1（SDA/SCL），GP28をADC2に割り当てます．**Pico WではGP23〜25は基板内の用途があるため，外部GPIOとしての利用を想定しないでください**．GP0〜15はLEDフレームバッファ有効時に占有されます．ピン設定やビルド構成を変更すると，公開されるノードも変化します．

## 4．`/tmp`と`/etc`の上限

| 項目 | 上限・動作 |
|---|---|
| ファイル数 | `/tmp`と`/etc`で**合計11個** |
| ファイル容量 | **各2048バイト** |
| 名前 | 各ディレクトリ直下のみ．ファイル名は**最大27バイト**（ASCII想定．`tmp/`または`etc/`を含む内部パスは最大31バイト） |
| 永続性 | なし．再起動で初期化 |
| 初期ファイル | Pico：`/etc/rc`，`/tmp/test.s`，`/tmp/test.py`，`/tmp/wave.py`（4個）．Pico W：これらに`/etc/index.html`，`/etc/fbcheck.html`，`/etc/pwm.html`を追加（7個） |
| 作成・削除 | `O_CREAT`で作成，`unlink()`で削除．開いたままのファイルは削除不可 |
| 書き込み | 容量末尾では部分書き込み，空きがない場合は`OS_ENOSPC` |

ファイル名・長さなどの管理情報はカーネルRAMに置き，11個の2 KiBデータ領域は`0x20032800`から連続配置します．詳細は[メモリレイアウト](MEMORY-LAYOUT.md)を参照してください．

`/etc/index.html`はユーザ空間の`httpd`が配信するページです．RAM上で編集できますが，変更はFlashに保存されません．`/tmp`と`/etc`の下にさらにディレクトリを作ることはできません．

## 5．パスとFD

相対パスはタスクごとのカレントディレクトリを基準に解決します．`.`と`..`の正規化に対応しますが，シンボリックリンクや任意のディレクトリ作成を提供するものではありません．1タスクのファイルディスクリプタ数は16です．パイプやソケットもFDを消費します．詳しくは[パイプ](../literacy/PIPE.md)，[ソケットプログラミング](../development/apps/c/NETWORK-PROGRAMMING.md)，[仕様書](SPECIFICATION.md)を参照してください．

## 6．実装上の参照先

- `kernel/kernel.c`：`vdir_paths`，`dev_registry`，`vfs_registry`，`vdir_nth()`，`vfs_open()`，`tmp_*()`．
- `kernel/pins.h`：GPIO・ADC・PWM・UART・I²Cの割り当て．
- `include/piconix.h`：公開APIとエラー定数．

初期ファイルのホスト側配置，追加方法，起動時復元については[初期RAMファイルと /etc/rc](INITIAL-FILES.md)を参照してください．
