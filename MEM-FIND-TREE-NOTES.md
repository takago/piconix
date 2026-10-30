# PicoNIX mem / find / tree update

Based on the previous remove-/proc/syms patch (apply that patch first).

- mem: `g` in interactive mode prompts for a hexadecimal address (8 hex digits maximum, `0x` omitted). Enter jumps; Esc cancels. Existing arrow/PgUp/PgDn/space/q behavior is preserved.
- /proc/ramfiles: read-only live metadata with lines `XXXXXXXX name`, where the address is the actual start of the file data buffer, not the file record header.
- mem: `RAM_FILE: /tmp/name` is appended to a matching 16-byte row, after existing symbol names (comma-separated).
- tree: `tree [-L 0..8] [path]` recursively lists the virtual tree.
- find: `find [path] [-name pattern]` supports `*` and `?`, with recursion limited to 8 levels.
- No new system calls, no dynamic memory allocation. Directory enumeration is shared between tree/find in userland/walk.h; existing ls implementation is unchanged except for showing /proc/ramfiles.

## Verification

- Host-side syntax checks passed for mem/find/tree/help/ls with gcc `-fsyntax-only`.
- Host Python tests passed except test_ram_pipeline.py, which cannot run without arm-none-eabi-gcc and libgcc.
- Pico SDK build, PIC checking, binary size, and physical-board operation were **not verified** in this environment. Run build_sdk.sh on the target toolchain and test RAM bounds.
- The directory enumeration uses existing PicoNIX virtual directory conventions; no POSIX opendir/readdir is introduced.

## Suggested manual checks

```
ls /proc
cat /proc/ramfiles
tree /
tree -L 2 /proc
find /tmp
find /proc/sym -name sh
mem
```
In mem, press `g`, enter a valid SRAM address, press Enter. Press `g`, Esc to cancel. Create a /tmp file with edit and navigate to its address from /proc/ramfiles to verify RAM_FILE label.
