# edit and mem

## edit — small screen editor

Run `edit /tmp/test.s` to edit or create a file. Use arrow keys to move, Enter for a newline, and TAB for 4-column tab stops (the TAB byte is preserved). Ctrl+S saves and Ctrl+Q exits; unsaved changes prevent quitting. Files are limited to 1,024 bytes; 20 lines are shown. Horizontal scrolling is not implemented.

## mem — live read-only memory viewer

Run `mem` or `mem 0x20018000`. This reads existing `/dev/mem` approximately every 200 ms and highlights changing bytes. Up/Down scroll by 8 bytes; PgUp/PgDn by a page; SPACE pauses and resumes; q exits.

Accessible ranges are the XIP Flash (0x10000000–0x101fffff) and SRAM (0x20000000–0x2003ffff) regions supported by `/dev/mem`. Concurrent modifications may prevent consistent snapshots.

Use `mem -s` to list symbol sources, and `mem -s sh` or `mem -s kernel` to list application or kernel symbols. The interactive memory viewer displays relevant symbols for the current address. Keys `1`–`6` select 24 KiB task slots, `k` selects kernel RAM, `K` selects XIP Flash, and `g` prompts for a hexadecimal address. Memory browsing continues if symbols are unavailable.
