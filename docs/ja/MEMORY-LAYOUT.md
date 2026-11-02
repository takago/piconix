# RP2040 SRAMレイアウト（現行配置）

| 領域 | アドレス範囲 | サイズ |
|---|---|---:|
| Kernel | `0x20000000..0x2000DFFF` | 56 KiB |
| User slots | `0x2000E000..0x20031FFF` | 6 × 24 KiB |
| SHM | `0x20032000..0x200327FF` | 2 KiB |
| RAM files | `0x20032800..0x20037FFF` | 11 × 2 KiB |
| Trace | `0x20038000..0x2003BFFF` | 16 KiB |
| Network | `0x2003C000..0x2003FFFF` | 16 KiB |

RAMファイルの管理テーブルはKernel内に残し，実データだけを専用領域に置く．`/proc/ramfiles`は実データの開始アドレスを表示する．`/dev/mem`経由の書き込みではファイル長は更新されない．

`/dev/shm`は2 KiBの連続した共有RAMデバイスで，`read`，`write`，`lseek`（SEEK_SET）に対応する．同じ実体に`/dev/mem`やPICプログラムの絶対アドレスでもアクセスできる．起動時にSHMをゼロクリアする．

`mem`では各領域を識別でき，`trace`は新しい16 KiBリングを使用する．

**確認状況:** Pico W版のELF配置監査はPASSし，実機で`/proc/memory`，`/proc/ramfiles`，`/dev/shm`，`/dev/mem`，`mem`，`trace`の動作を確認済みです．ネットワーク通信も動作報告がありますが，AP/STA両モードの網羅的な安定性試験を意味しません．
