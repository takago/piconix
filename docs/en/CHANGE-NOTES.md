# Program Capacity and Terminal Display Improvements

- The program registry in `kernel/kernel.c` now accepts IDs 1 through 64, with ID 0 reserved. This does not increase the number of concurrently running tasks. New apps still require a registration in `kernel/main.c` and an entry in `CMakeLists.txt`.
- `edit` requests a cursor-position report (`CSI 6n`) at startup to determine terminal dimensions, and falls back to 80x24 when unsupported. Its visible rows and horizontal scrolling adapt to terminal width; the 1 KiB file limit is unchanged.
- `mem` shows 16 rows of 16 bytes (256 bytes total), with hex and ASCII columns, a region/status header, and inverse-video change highlighting. `k` jumps to kernel SRAM, `1`–`5` to task slots, and `f` to XIP flash. Arrow keys step by 16 bytes and PageUp/PageDown by 256 bytes.

**Note:** `edit` uses 80x24 on terminals that do not answer cursor-position queries. `/dev/mem` access remains subject to kernel restrictions.
