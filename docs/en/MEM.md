# mem — read-only memory viewer

`mem` inspects SRAM and XIP Flash; **it does not edit memory**.

Run `mem` or `mem 0x20018000`. This reads existing `/dev/mem` approximately every 200 ms and highlights changing bytes. The controls are summarized below.

Accessible ranges are the XIP Flash (0x10000000–0x101fffff) and SRAM (0x20000000–0x2003ffff) regions supported by `/dev/mem`. Concurrent modifications may prevent consistent snapshots.

Use `mem -s` to list symbol sources, and `mem -s sh` or `mem -s kernel` to list application or kernel symbols. The interactive memory viewer displays relevant symbols for the current address.

| Key | Action |
|---|---|
| `↑` / `↓` | Move by 8 bytes |
| `PgUp` / `PgDn` | Move by a screen |
| `SPACE` | Pause or resume updates |
| `1`–`6` | Jump to the corresponding 24 KiB task slot |
| `k` / `K` | Jump to kernel RAM / XIP Flash |
| `g` | Enter a hexadecimal address |
| `q` | Quit |

Memory browsing continues if symbols are unavailable.
