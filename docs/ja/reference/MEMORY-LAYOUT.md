# RP2040 SRAMレイアウト（現行配置）

*技術リファレンス*

| 領域 | アドレス範囲 | サイズ |
|---|---|---:|
| Kernel | `0x20000000..0x2000DFFF` | 56 KiB |
| User slots | `0x2000E000..0x20031FFF` | 6 × 24 KiB |
| SHM | `0x20032000..0x200327FF` | 2 KiB |
| RAM files | `0x20032800..0x20037FFF` | 11 × 2 KiB |
| Trace | `0x20038000..0x2003BFFF` | 16 KiB |
| Network | `0x2003C000..0x2003FFFF` | 16 KiB |

RAMファイルの管理テーブルはKernel内に残し，実データだけを専用領域に置く．`/proc/ramfiles`は実データの開始アドレスを表示する．`/dev/mem`経由の書き込みではファイル長は更新されない．

`/dev/shm`は2 KiBの連続した共有RAMデバイスで，`read`，`write`，`lseek`（SEEK_SET / SEEK_CUR / SEEK_END）に対応する．同じ実体に`/dev/mem`やPICプログラムの絶対アドレスでもアクセスできる．起動時にSHMをゼロクリアする．

`mem`では各領域を識別でき，`trace`は新しい16 KiBリングを使用する．

**確認状況:** Pico W版のELF配置監査はPASSし，実機で`/proc/memory`，`/proc/ramfiles`，`/dev/shm`，`/dev/mem`，`mem`，`trace`の動作を確認済みです．ネットワーク通信も動作報告がありますが，AP/STA両モードの網羅的な安定性試験を意味しません．

## Pico Wのネットワーク領域

16 KiBのネットワーク予約領域を，lwIPヒープ10 KiB（`0x2003C000..0x2003E7FF`），
境界用の2 KiB（`0x2003E800..0x2003EFFF`），アイドル・通信処理用スタック4 KiB
（`0x2003F000..0x2003FFFF`）に分けます．境界領域にはlwIPヒープの管理情報も
含まれるため，2 KiBすべてを空き領域として使えるわけではありません．
通常のPicoではアイドルスタックを従来どおりカーネルRAMに配置します．
静的なpbufプール，PCBプール，CYW43管理領域は引き続きカーネルRAMを使います．
アイドルスタックの移動によって，カーネルRAM側の使用量を4 KiB削減します．
`cat /proc/net/memory`でヒープとプールの使用ピーク・確保失敗回数を確認できます．
Pico Wではリンク後に固定アドレス配置を必ず検査してください．

## 共有メモリを利用するアセンブリ例

`/dev/shm`の先頭4バイトを32ビットの共有カウンタとして利用する例です．アセンブリから物理アドレス`0x20032000`を直接参照し，カウンタを1増やします．プログラムを終了して再実行しても値は保持されますが，再起動時にはゼロに初期化されます．

```asm
@ Increment shared 32-bit counter at physical address 0x20032000.
@ Run repeatedly; each process sees the same shared memory.
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

例えば`/tmp/counter.s`として保存し，次を実行します．

```sh
as counter.s
./a.out
./a.out
./a.out
dd if=/dev/shm of=- bs=1 count=4 | hexdump
```

共有メモリの先頭が最初にゼロなら，3回の実行後は`03 00 00 00`（リトルエンディアン）になります．`hexdump`が16ビット単位で表示する場合は`0003 0000`のように見えます．`as`は`/tmp/a.out`へ出力します．

**注意:** `ldr`・加算・`str`は一体の不可分操作ではありません．複数プロセスが同時に実行すると更新が失われることがあります．この例は順番に実行して共有領域を確かめる教材です．`/dev/shm`は他のプログラムとも共用されるため，先頭4バイトを別用途に使っている場合は上書きしないでください．

---

**関連リンク：** [リファレンス目次](README.md) ｜ [学習ロードマップ](../ROADMAP.md) ｜ [English](../../en/reference/MEMORY-LAYOUT.md)
