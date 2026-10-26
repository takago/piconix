# PicoNIX

**PicoNIX** は，Raspberry Pi Pico（RP2040）で動作する，UNIXの基本的な仕組みを学ぶための実験用OSです(ChatGPTで作成)．

> リリース表示：`PicoNIX-2026-10-26 (TAKAGO_LAB.)`  
> 開発段階：Stage 30  
> ライセンス：GNU General Public License v3.0 or later（GPL-3.0-or-later）

Pico SDKを利用したカーネルと，固定RAMスロットに配置する独立したShell・サンプルプログラムから構成されます．通常のLinuxディストリビューションではなく，OSの内部構造を観察し，改造することを目的とした教材・研究用の実装です．

## 特徴

- **固定RAMスロット**：カーネル，Shell，Blink，Omikuji／Tiny Pythonを固定の64 KiB領域に配置します．OmikujiとTiny Pythonは同じスロット3を共有し，同時実行できません．
- **マルチタスク**：SysTick，PendSV，SVCを利用したタスク切り替えとシステムコール．
- **タスク管理**：PID，`ps`，`kill`，`exit`，`waitpid`相当の機能．
- **簡易ジョブ制御**：コマンド末尾の`&`でバックグラウンド起動．フォアグラウンドでは終了待ちし，Ctrl+Cで中断できます．
- **USB CDCコンソール**：PicoのUSB端子を通じてShellを操作します．
- **ファイル風インターフェース**：`/dev`と`/sys/proc`に加え，`/tmp`のRAM上の通常ファイルを操作できます．
- **教育用サンプル**：LED点滅の`blink`と，標準入力・カウントダウン・正常終了を確認できる`omikuji`．

## 起動画面

```text
--------------------------------------------------------
PicoNIX-2026-10-26 (TAKAGO_LAB.)
Type 'help' for commands.
--------------------------------------------------------
piconix# 
```

`#`はUNIXの特権ユーザのプロンプトを意識した表記です．ただし，現在のPicoNIXにはユーザアカウントや権限分離はありません．USB CDCの接続タイミングによっては，リセット直後の文字がPC側に届かない場合があります．

## 動作例

```text
piconix# help
piconix# ps
piconix# omikuji
Countdown seconds (1-30): 5
5...4...3...2...1... (*^_^*) DAIKICHI!
piconix# blink &
[2] blink started
piconix# ps
piconix# kill 2
```

上記は操作例です．PIDやおみくじの結果は実行ごとに異なります．

| コマンド | 説明 |
|---|---|
| `help` | コマンド一覧を表示 |
| `ps` / `pid` | タスク一覧／自身のPID |
| `blink` / `blink &` | LED点滅を前景／背景で実行 |
| `omikuji` | 前景で秒数を標準入力から受け取る |
| `omikuji 5` / `omikuji 5 &` | 5秒のカウントダウンを前景／背景で実行 |
| `omikuji &` | 既定の10秒で背景実行 |
| `kill PID` | タスクを終了 |
| `ls /dev` / `ls /sys/proc` | デバイス／統計項目を一覧表示 |
| `cat PATH` / `cat > PATH` | 読み取り／標準入力からファイルを作成（Ctrl+Dで終了） |
| `echo TEXT` / `echo TEXT > PATH` / `echo TEXT >> PATH` | 表示／上書き／追記 |
| `rm PATH` | `/tmp` のファイルを削除 |
| `python` / `python PATH` | Tiny Pythonの対話モード／指定スクリプト実行 |
| `COMMAND < FILE` | 標準入力をファイルに切り替え |
| `COMMAND > FILE` / `COMMAND >> FILE` | 標準出力の上書き／追記 |

`run`コマンドは廃止されています．前景実行中のCtrl+Cは対象タスクを中断します．背景タスクはUSBコンソールの標準入力を取得できず，コンソールからの読み取りはエラーになります．背景タスクの出力はShellの入力表示と混ざることがあります．

### GPIOとUARTの例

```text
piconix# echo out > /dev/gpio/25.dir
piconix# echo 1 > /dev/gpio/25
piconix# echo 0 > /dev/gpio/25
piconix# cat /dev/gpio/25.dir
piconix# echo up > /dev/gpio/16.pull
piconix# cat /sys/proc/uptime
piconix# cat /sys/proc/memory
```

RP2040搭載の通常のRaspberry Pi PicoではGPIO25がオンボードLEDです．UART1はGP4（TX）／GP5（RX）を使用します．`cat /dev/uart1`は受信が約200 ms途絶えると終了する簡略仕様です．配線には3.3 Vロジックを使用してください．

## セットアップ

UbuntuなどのLinux環境で，Pico SDK 2.1.1，ARM GCC，CMake，Python 3などを使用します．詳細は **[SETUP.md](SETUP.md)** を参照してください．

```bash
chmod +x ./tools/build_sdk.sh
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
LC_ALL=C LANG=C \
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
./tools/build_sdk.sh
```

成功すると通常は`build/piconix.uf2`が生成されます．内部のビルドターゲット名は歴史的理由で`piconix`のままです．UF2をPicoのBOOTSELモードで書き込んでください．

## 内部構造

```text
RP2040 SRAM
  0x20000000–0x2000FFFF  Kernel（64 KiB）
  0x20010000–0x2001FFFF  Shell（64 KiB）
  0x20020000–0x2002FFFF  Blink（64 KiB）
  0x20030000–0x2003FFFF  Omikuji / Tiny Python（共有64 KiB）
```

各外部プログラムは固定アドレスにリンクされます．ELFローダや再配置機能はありません．実装の詳しい説明は **[INTERNAL.md](INTERNAL.md)** にまとめています．

## ディレクトリ構成

| パス | 内容 |
|---|---|
| `src/` | カーネル，起動処理，コンテキスト切り替え |
| `include/` | カーネルAPI，定数，データ型 |
| `ram_apps/` | Shell，Blink，Omikuji，Tiny Pythonと固定スロット用リンカスクリプト |
| `tools/` | SDKビルド，配置監査，回帰テスト |
| `.github/workflows/` | CI用ビルドワークフロー |

## 自作プログラムの作成

自作のRAMプログラムを作る場合は，**[PROGRAMMING.md](PROGRAMMING.md)** を参照してください．Blinkを改造する最初の演習から，SVCによるシステムコール，固定スロットへのリンク，CMake・カーネル・Shellへの登録方法まで説明しています．

## 制限事項

- 対象はRP2040搭載の通常のRaspberry Pi Picoです．Pico WやRP2350での動作は保証していません．
- タスクスロットは固定で，同じスロットに複数のプログラムを同時配置できません．
- `waitpid`は終了記録を利用する簡易実装です．Shell側の待機はスリープを挟むポーリングであり，完全なUNIX互換ではありません．
- プロセスグループ，シグナル，パイプ，ファイルシステムの永続化，ユーザ権限管理は未実装です．標準入出力のリダイレクトは簡易実装です．
- Omikujiの秒数引数は専用の簡易機構であり，一般的な`argc/argv`ではありません．
- 学習・実験用の実装であり，製品用途や安全性を要する用途を想定していません．

## ライセンス

PicoNIXのプロジェクト独自コードは **GPL-3.0-or-later** で公開します．全文は[LICENSE](LICENSE)を参照してください．Pico SDKその他の外部依存物には，それぞれのライセンスが適用されます．既存コードの権利関係や外部由来コードがある場合は，公開前に確認してください．

### 起動時のメッセージ

USB CDC接続が確立した後，Shellが起動バナーを一度表示します．端末を接続してからPicoをリセットすると確認できます．

```text
--------------------------------------------------------
PicoNIX-2026-10-26 (TAKAGO_LAB.)
Type 'help' for commands.
--------------------------------------------------------
piconix# 
```


### 疑似乱数デバイス `/dev/rand`

`/dev/rand`は，1回の`read(fd, &value, 4)`で32ビットの疑似乱数を1個返します．
繰り返し`read()`すると新しい値を返します．4バイト以外の読み取りはエラーです．
書き込みはできません．暗号用途には使用できません．
`omikuji`はこのデバイスから運勢決定用の値を取得します．
通常のUNIXの`/dev/random`や`/dev/urandom`とは異なるPicoNIX独自の簡略仕様です．

`ps` reads `/sys/proc/tasks` through ordinary `open()`/`read()` calls.
The former task-info system call (number 11) is retired and reserved.

`ls /dev` obtains its fixed device list from the kernel's `/sys/proc/devices`
virtual file, using the same device registry as `open()`.

USB console writes may return a partial count when the kernel transmit queue
is full. User programs should retry the remaining bytes (and yield/sleep if
zero bytes were accepted).

### カレントディレクトリと相対パス

Shellは起動時に`/`をカレントディレクトリとします．`pwd`で表示し，`cd PATH`で移動できます（`cd`のみなら`/`へ戻ります）．`ls`は引数を省略するとカレントディレクトリを表示します．`cat`および`echo TEXT > PATH`も相対パスに対応します．`.`，`..`，重複した`/`を解釈します．

```text
piconix# ls
dev/
sys/
tmp/
piconix# cd /sys/proc
piconix# pwd
/sys/proc
piconix# cat memory
piconix# cd ../../dev
piconix# ls
```

カレントディレクトリはShell内部だけで保持します．カーネルの`open()`は引き続き絶対パスを受け取り，子タスクへカレントディレクトリは継承されません．`cd`できるのは既存の仮想ディレクトリ（`/`，`/dev`，`/dev/gpio`，`/sys`，`/sys/proc`，`/tmp`）のみです．

### RAMファイル（`/tmp`）

`/tmp` は再起動で消える固定容量のRAMファイル領域です．最大4ファイル，各512バイト，ファイル名は最大15文字（階層構造なし）です．

```text
piconix# echo hello > /tmp/test.txt
piconix# echo world >> /tmp/test.txt
piconix# cat /tmp/test.txt
hello
world
piconix# ls /tmp
test.txt
piconix# rm /tmp/test.txt
```

`cat > /tmp/hello.py` では複数行を入力でき，空行でのCtrl+Dで終了します．通常の行入力ではEnterを使用します．途中まで入力した行でCtrl+Dを押すと，その行を確定して返します．終了にはもう一度，空行でCtrl+Dを入力してください．開いたままのファイルを削除しようとするとエラーを返します．

`open()` は `O_CREAT`（8），`O_TRUNC`（16），`O_APPEND`（32）を追加しています．`unlink()` はSVC番号17です．

### 入出力リダイレクト

Shellの組み込みコマンドにも，`spawn()`で起動する外部プログラムにも `<`，`>`，`>>` を指定できます．ファイル名の前後の空白・タブは無視します．

```sh
ps > /tmp/tasks.txt
cat /dev/uart1 > /tmp/capture.txt
cat < /tmp/tasks.txt > /tmp/copy.txt
omikuji 5 > /tmp/result.txt &
```

タスクにはFD 0（標準入力），FD 1（標準出力），FD 2（標準エラー出力）があり，リダイレクトは`dup2`（システムコール番号18）を使って実現します．`spawn()`では標準FDを引き継ぎます．終了後，Shellの入出力先は元に戻ります．FDの継承・複製はLinuxと完全互換ではありません（ファイル位置を共有しません）．パイプ，引用符付きパス，`2>` は未対応です．詳しくは[PROGRAMMING.md](PROGRAMMING.md)を参照してください．

### Tiny Python

`python` はREPLを起動し，`python /tmp/name.py` は指定したスクリプトを実行します．REPLでのみPythonサブセットであることを示すバナーを表示します．GPIO操作，整数演算，`if`，`while`，`input()` 等に対応します．本家PythonやMicroPythonとは互換ではありません．詳細は **[PYTHON.md](PYTHON.md)** を参照してください．

Background execution (`&`) is parsed once by the shell before `<`, `>`,
and `>>`, for all commands. It is supported for spawned applications
(`blink`, `omikuji`, and script-based `python`); shell builtins and
interactive `python &` cannot run in the background.
