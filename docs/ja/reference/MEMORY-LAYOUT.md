# RP2040メモリレイアウト（ROM・Flash・SRAM）

*技術リファレンス — 不揮発メモリと揮発メモリ，固定予約領域と実使用量を区別する*

## ROMとFlash（不揮発メモリ）

RP2040には**内蔵Boot ROM（16 KiB）**があり，リセット時の起動処理などを提供します．これはPicoNIXのファームウェアを格納する領域ではありません．Pico/Pico Wでは通常，**基板上の外付け2 MiB QSPI Flash**にPicoNIXを保存し，RP2040はこれをXIP（Execute in Place）ウィンドウへマップして実行します．内蔵Boot ROMと外付けFlashを混同しないでください．

| 領域・格納内容 | アドレス／配置 | 容量・用途 | 実行時の扱い |
|---|---|---|---|
| 内蔵Boot ROM | `0x00000000..0x00003FFF` | 16 KiB．RP2040内蔵の起動コード（外付けFlashではない） | リセット時の起動処理 |
| 外付けFlash（XIP領域全体） | `0x10000000..0x101FFFFF` | 標準Pico/Pico Wでは2 MiB．以下の各要素を収容 | XIP経由で読取り |
| 第2段階ブートローダ | `.boot2`：`0x10000000..0x100000FF` | 256バイト．SDKが生成 | 起動時にFlashアクセスを設定 |
| カーネル・Pico SDK等の実行コード | 主に`.text`（リンカ配置） | サイズ・開始位置はビルド依存 | FlashからXIP実行 |
| 定数・初期値 | 主に`.rodata`と`.data`のFlash側初期値 | 文字列，定数，SRAM初期化用データなど | 定数はFlashで参照，`.data`は起動時にSRAMへコピー |
| 組込みユーザアプリ | `.piconix_app_images`内のPICイメージ | アプリごとのバイナリ．サイズはビルド依存 | 起動時にSRAMのユーザスロットへコピーして実行 |
| アプリ・カーネルのシンボル表 | `.piconix_app_images`内のシンボルデータ | サイズはビルド依存 | シンボル名の参照に使用 |
| ROMマニュアル | `.manual` | インデックスとマニュアル本文 | Flash上から読み出す |
| 初期RAMファイルの内容 | 読取専用の埋込みデータ（リンカ配置） | 起動時のファイル初期内容 | SRAMのRAMファイル領域に展開 |
| 未使用領域 | ファームウェアイメージ末尾より後 | 2 MiBから使用領域を除いた範囲 | PicoNIXの通常のファイル書込み領域ではない |

`.boot2`以外の内訳は**固定アドレスで区切った予約領域ではありません**．リンカは通常の`.text`・`.rodata`に加え，組込みアプリとシンボル表を`.piconix_app_images`，ROMマニュアルを`.manual`に配置します．カーネルはFlash上でXIP実行しますが，**ユーザアプリはFlash内のPICイメージを起動時にSRAMスロットへコピーして実行**します．初期RAMファイルもビルド時にFlashへ組み込まれ，起動時にRAMへ展開されます．`.data`の初期値もFlashに保持され，起動処理でSRAMへコピーされます．

- ROMマニュアル：`manual/`，`tools/make_manual_rom.py`，`cmake/embed_manual.S.in`，`kernel/manual.h`
- 組込みアプリとシンボル表：`CMakeLists.txt`，`cmake/embed_image.S.in`，`kernel/section_extra_post_text.incl`
- Flash配置と監査：`tools/audit_firmware.py`，`tools/size_ram.py`

上記の2 MiBは標準Pico/Pico WのFlash容量に対応する**この版の監査・閲覧対象範囲**です．RP2040のXIPアドレス空間全体の大きさと同じではありません．`/dev/mem`と`mem`がFlashを参照できる範囲も`0x10000000..0x101FFFFF`に制限されています．Flashの内容をPicoNIXの通常のファイル書込み操作で永続更新する機能はありません．

ビルド後の`python3 tools/size_ram.py`で，Flashに割り当てられた総量と，PicoNIX，組込みアプリ，ROMマニュアル，シンボル表，SDK，lwIP，CYW43などの**リンクマップに基づく概算内訳**を確認できます．これはFlashの全容量でも空き領域の厳密な保証値でもありません．各セクションの実アドレスは`piconix.elf`とリンカマップで確認します．

## SRAM（揮発メモリ）

PicoNIXはRP2040の**256 KiB主SRAM**（`0x20000000..0x2003FFFF`）を以下のように分割します．この表のサイズは**予約容量**であり，ELFが実際に占める容量や実行時の空きメモリではありません．PicoとPico Wで主SRAMの固定領域の境界は共通ですが，ネットワーク領域の使い方とアイドルスタックの配置は異なります．

| 領域 | アドレス範囲（両端を含む） | 予約容量 | 用途 |
|---|---|---:|---|
| Kernel | `0x20000000..0x2000DFFF` | 56 KiB | カーネルのデータ，管理情報，スタックなど |
| User slots | `0x2000E000..0x20031FFF` | 144 KiB | 24 KiB × 6プロセス用スロット |
| SHM | `0x20032000..0x200327FF` | 2 KiB | `/dev/shm`共有メモリ |
| RAM files | `0x20032800..0x20037FFF` | 22 KiB | 11ファイル × 2 KiBの実データ |
| Trace | `0x20038000..0x2003BFFF` | 16 KiB | トレース記録リング |
| Network reserve | `0x2003C000..0x2003FFFF` | 16 KiB | Pico WのlwIPヒープ等 |

RP2040には，この主SRAMとは別に**8 KiBのスクラッチSRAM**（`0x20040000..0x20041FFF`）もあります．上表の256 KiBには含めません．また，カーネルのコードは原則としてFlashのXIP領域から実行されるため，Kernelの56 KiBがコード全体のサイズという意味でもありません．

## ユーザスロットとスタック

ユーザプロセスは最大6スロットで，各24 KiBの固定領域を持ちます．その**末尾4 KiBはスタック予約**で，コード・初期化データ・BSSが利用できるのは残り最大20 KiBです．リンカは`__bss_end <= __stack_bottom`を検査します．スロット数とサイズは`include/piconix.h`，配置と起動処理は`kernel/kernel.c`，ユーザイメージの境界は`userland/lib/slot.ld`で確認できます．「スロット24 KiB」はアプリケーションのファイルサイズ上限24 KiBを意味しません．

## RAMファイル，共有メモリ，トレース

`/tmp`と`/etc`のRAMファイルは，共通の**11個の固定枠**（各2 KiB）を使います．ファイル名，長さ，使用状態を管理するテーブルはKernel領域にあり，実データだけをRAM files領域に置きます．`/proc/ramfiles`で配置を確認できます．`/dev/mem`から内容を直接書き換えてもファイル長は変化しません．

`/dev/shm`は2 KiBの共有バイト領域で，`read()`，`write()`，`lseek()`（`SEEK_SET`，`SEEK_CUR`，`SEEK_END`）に対応します．アプリケーションは物理アドレス`0x20032000`を直接参照することもできます．SHMは起動時にゼロクリアされます．`trace`の記録領域は`0x20038000`から始まる16 KiBのリングです．

## PicoとPico Wの違い

Pico WではNetwork reserveを次のように使用します．

| 用途 | アドレス範囲 | 容量 |
|---|---|---:|
| lwIPヒープ本体 | `0x2003C000..0x2003E7FF` | 10 KiB |
| ヒープ管理情報などの境界余裕 | `0x2003E800..0x2003EFFF` | 2 KiB |
| アイドルスタック | `0x2003F000..0x2003FFFF` | 4 KiB |

2 KiBの境界領域は全体が自由に割り当てられる空きメモリではありません．lwIPの静的pbuf/PCBプールやCYW43管理データは，この専用領域とは別にKernel側のSRAMも消費します．通常のPicoではアイドルスタックはKernel領域内に置かれます．Pico Wにある上記ネットワーク用途を通常のPicoにも当てはめてはいけません．

## 配置・使用量の確認

開発ホスト上でビルドした後，次を実行します．

```sh
python3 tools/size_ram.py
```

このスクリプトは固定配置の予約容量とELFに基づく割当量を別々に報告します．アプリのコード・データ・BSS・予約スタック・残量も表示します．固定アドレスと重複の検証は`tools/verify_layout.py`，Pico WのELF監査は`tools/audit_wifi_elf.py`と`tools/audit_firmware.py`が担います．**配置検査の成功は，実行時のスタック安全性やネットワークの安定性を保証しません**．

PicoNIX上では次の観察ができます．

```sh
cat /proc/memory
cat /proc/ramfiles
cat /proc/net/memory
mem
```

`/proc/net/memory`はPico Wのネットワークメモリの使用状況を調べるためのものです．実装は`kernel/net.c`，メモリ・プロセス情報は`kernel/kernel.c`を参照してください．

## 共有メモリを利用するアセンブリ例

次は`/dev/shm`の先頭4バイトを32ビットの共有カウンタとして増加させます．同じ物理アドレスを各プロセスから参照でき，実行を終了しても値が残りますが，再起動するとゼロに初期化されます．

```asm
@ Increment shared 32-bit counter at physical address 0x20032000.
        adr     r2, shm_addr
        ldr     r2, [r2]
        ldr     r3, [r2]
        adds    r3, #1
        str     r3, [r2]
        movs    r0, #3
        movs    r1, #0
        svc     #0
        .align  2
shm_addr:
        .word   0x20032000
```

`/tmp/counter.s`として保存して，次のように実行します．

```sh
as counter.s
./a.out
./a.out
./a.out
dd if=/dev/shm of=- bs=1 count=4 | hexdump
```

初期値がゼロであれば，3回実行後の先頭4バイトはリトルエンディアンで`03 00 00 00`です．`hexdump`の表示単位によって見え方は異なります．**ロード・加算・ストアは不可分ではない**ため，同時実行による競合には対応していません．共有領域をほかの用途に使用している場合は上書きしないでください．

---

**関連リンク：** [リファレンス目次](README.md) ｜ [日本語文書案内](../../DOCS.ja.md) ｜ [English](../../en/reference/MEMORY-LAYOUT.md)
