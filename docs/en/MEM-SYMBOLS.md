# `mem` symbol view (experimental)

The `mem` command can read a compact application symbol catalog extracted from the RAM app ELF files during the firmware build. The catalog is embedded in the firmware and exposed through read-only `/proc/syms`.

```text
mem -s             # Applications with symbols
mem -s sh          # Show sh's symbol offsets and types
mem -s mb          # Show mb's symbol offsets and types
mem 0x20018000     # View memory at an address
mem 0x20018000 sh  # Show sh symbol labels right of ASCII
```

`mem -s kernel` is not implemented yet: kernel symbols require an additional post-link strategy.

Symbol values are **relative offsets** from the app's canonical link address `0x20018000`. For `mem ADDRESS APP`, labels assume that APP is loaded in the 32-KiB task slot containing ADDRESS; the process table is not consulted to verify this. Choosing the wrong app produces misleading labels. Only symbol *starts* located within a displayed row are annotated.

If `/proc/syms` is missing, unreadable or empty, `mem -s` prints an error and exits cleanly. Ordinary `mem [ADDRESS]` continues to work without symbol data. The viewer still restricts reads to accessible XIP Flash and SRAM. The extra label column requires a sufficiently wide terminal.

The build tool `tools/make_symbols.py` extracts defined `T/t/D/d/B/b/R/r` symbols from linked app ELF files and embeds them as a compact text catalog. Symbol-less apps are permitted. The catalog is exposed read-only.
