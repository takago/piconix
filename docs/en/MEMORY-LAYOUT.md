# RP2040 SRAM Layout (current layout)

| Region | Address range | Size |
|---|---|---:|
| Kernel | `0x20000000..0x2000DFFF` | 56 KiB |
| User slots | `0x2000E000..0x20031FFF` | 6 × 24 KiB |
| SHM | `0x20032000..0x200327FF` | 2 KiB |
| RAM files | `0x20032800..0x20037FFF` | 11 × 2 KiB |
| Trace | `0x20038000..0x2003BFFF` | 16 KiB |
| Network | `0x2003C000..0x2003FFFF` | 16 KiB |

File metadata remains in kernel RAM; only the 11 fixed 2-KiB payloads are relocated. `/proc/ramfiles` reports their data addresses. Direct `/dev/mem` writes do not update file lengths.

`/dev/shm` exposes a common 2-KiB byte-addressed RAM area with `read`, `write` and `lseek` (SEEK_SET). PIC applications can access the same physical region directly. SHM is cleared on boot.

The `mem` viewer labels the relocated regions, and the `trace` ring is at the new 16-KiB address.

**Validation status:** Pico W ELF layout audit passed. Hardware checks confirmed `/proc/memory`, `/proc/ramfiles`, `/dev/shm`, `/dev/mem`, `mem`, and `trace`. Network communication has also been reported working, but exhaustive AP/STA stability testing is not established.
