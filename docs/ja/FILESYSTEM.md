# ファイルシステムとデバイスノード

[English](../en/FILESYSTEM.md) | [ドキュメント案内](DOCS.md) | [デバイス操作](DEVICES.md)

PicoNIXは小さな仮想ファイルシステム（VFS）を提供します．**ディレクトリやデバイスノードはカーネルが生成し，通常のディスク上には存在しません**．`/tmp`と`/etc`の通常ファイルはSRAM上に保存され，再起動で失われます．この文書は現行の`kernel/kernel.c`のVFS登録・列挙処理と`kernel/pins.h`を基準にしています．

## 1．ディレクトリ構成

```text
/
├── bin/                    組込みユーザプログラム
├── dev/                    デバイス
│   ├── console             主端末
│   ├── uart1               UART1端末
│   ├── wifi                Wi-Fi端末（Wi-Fiビルドのみ）
│   ├── rand                簡易疑似乱数
│   ├── mem                 許可されたメモリ領域の読み出し
│   ├── trace               バイナリトレース
│   ├── gpio/
│   │   ├── fb              8×8 LEDフレームバッファ
│   │   ├── N               GPIO値
│   │   ├── N.dir           GPIO方向
│   │   └── N.pull          プルアップ／プルダウン設定
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
│   └── net/wifi            Wi-Fi情報（Wi-FiなしではMODE: N/A）
├── tmp/                    揮発性RAMファイル
└── etc/                    揮発性設定ファイル
    └── index.html          httpdの既定ページ
```

`N`，`PID`，`NAME`はプレースホルダーであり，その文字列のファイルが存在するわけではありません．`/bin`は登録済みの組込みプログラムを列挙します．`/proc/task`には**稼働中のPID**が列挙されます．`/proc/sym/userland`はプログラム名から直接ファイルを開けますが，現在のディレクトリ列挙処理では子要素を一覧表示しません．`/dev/pwm/1`はPWM使用時のみ有効です．

## 2．ノードの種類とアクセス

| ノード | 読み取り | 書き込み | 備考 |
|---|:---:|:---:|---|
| `/dev/console`, `/dev/uart1`, `/dev/wifi` | ○ | ○ | 端末．`/dev/wifi`はWi-Fiビルドのみ |
| `/dev/rand` | ○ | × | 暗号用途には使えない疑似乱数．1回の読み取りは最大4バイト |
| `/dev/mem` | ○ | × | XIP Flashと主SRAMの許可範囲．アドレスは`lseek()`で指定 |
| `/dev/trace` | ○ | × | バイナリレコード．`trace`コマンドを参照 |
| `/dev/gpio/fb` | ○ | ○ | 8バイト単位．有効化は`ioctl()`で行う |
| `/dev/gpio/N`, `N.dir`, `N.pull` | ○ | ○ | `pins.h`でGPIOに割り当てたピンのみ |
| `/dev/adc/0`〜`2`, `/dev/adc/temp` | ○ | × | 十進テキスト．0〜2はADC割り当て時のみ |
| `/dev/pwm/1/freq`, `a`, `b` | ○ | ○ | PWM割り当てに依存 |
| `/proc`配下の情報ファイル | ○ | × | カーネル状態のスナップショットなど |
| `/tmp/NAME`, `/etc/NAME` | ○ | ○ | RAM上の通常ファイル |
| ディレクトリ | 一覧取得 | × | `getdents()`で列挙．通常ファイルのような`read()`は不可 |

実際の読み書き形式，`ioctl()`の値，ピンの占有・排他条件は[デバイスガイド](DEVICES.md)と[TTY](TTY.md)を参照してください．`/proc/devices`は固定デバイスの登録表と`gpio/`，`adc/`，`pwm/`の入口を列挙するもので，すべての動的ノードを列挙するわけではありません．

## 3．GPIOと周辺機器の公開条件

標準の`kernel/pins.h`では，GP0〜17およびGP22〜25をGPIO，GP18/19をPWM，GP20/21をUART1，GP26〜28をADCに割り当てます．**Pico WではGP23〜25は基板内の用途があるため，外部GPIOとしての利用を想定しないでください**．GP0〜15はLEDフレームバッファ有効時に占有されます．ピン設定やビルド構成を変更すると，公開されるノードも変化します．

## 4．`/tmp`と`/etc`の上限

| 項目 | 上限・動作 |
|---|---|
| ファイル数 | `/tmp`と`/etc`で**合計8個** |
| ファイル容量 | **各1024バイト** |
| 名前 | 各ディレクトリ直下のみ．ファイル名は**最大11バイト**（ASCII想定） |
| 永続性 | なし．再起動で初期化 |
| 初期ファイル | `/etc/index.html`，`/tmp/test.s`，`/tmp/test.py`（計3スロット） |
| 作成・削除 | `O_CREAT`で作成，`unlink()`で削除．開いたままのファイルは削除不可 |
| 書き込み | 容量末尾では部分書き込み，空きがない場合は`OS_ENOSPC` |

`/etc/index.html`はユーザ空間の`httpd`が配信するページです．RAM上で編集できますが，変更はFlashに保存されません．`/tmp`と`/etc`の下にさらにディレクトリを作ることはできません．

## 5．パスとFD

相対パスはタスクごとのカレントディレクトリを基準に解決します．`.`と`..`の正規化に対応しますが，シンボリックリンクや任意のディレクトリ作成を提供するものではありません．1タスクのファイルディスクリプタ数は16です．パイプやソケットもFDを消費します．詳しくは[パイプ](PIPE.md)，[ソケットプログラミング](NETWORK-PROGRAMMING.md)，[仕様書](SPECIFICATION.md)を参照してください．

## 6．実装上の参照先

- `kernel/kernel.c`：`vdir_paths`，`dev_registry`，`vfs_registry`，`vdir_nth()`，`vfs_open()`，`tmp_*()`．
- `kernel/pins.h`：GPIO・ADC・PWM・UARTの割り当て．
- `include/piconix.h`：公開APIとエラー定数．

この文書はソースコードとの静的照合によるものです．実機で全ノードを読み書きする試験は行っていません．
