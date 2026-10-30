# Kernel symbols in `mem` (experimental)

`mem -s kernel` lists kernel function and data symbols using absolute addresses. The interactive `mem` view identifies symbols from the kernel and the currently loaded user program in each RAM slot and shows a header such as `SYMBOL: kernel, sh`. Multiple symbols within one 16-byte row are comma-separated.

A fixed 32-KiB region in XIP Flash stores the kernel catalog. Only a 4-byte length and the actual symbol records are exposed by `/proc/syms`. `tools/build_sdk.sh` performs an initial link, extracts kernel symbols, relinks with the catalog, and verifies that the final symbol addresses did not change. User program records remain slot-relative; kernel records have absolute addresses.

When symbol information is unavailable, interactive memory viewing continues without labels. Explicit symbol listing reports an error and exits.

Try `mem -s kernel`, `mem 0x20000000`, `mem 0x10000000`, and `mem -s sh`.

**Note:** Full ARM firmware building and hardware execution have not yet been verified for this experimental patch. Do not flash firmware if the post-link address verification fails.
