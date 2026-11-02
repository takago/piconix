# Kernel symbols in `mem` (current implementation)

`mem -s kernel` lists kernel functions and data symbols at absolute addresses. Interactive `mem` automatically selects symbols for the visible kernel and task-slot address ranges and displays labels such as `SYMBOL: kernel, sh`. Multiple symbols in a 16-byte row are separated by commas.

## Implementation

- A fixed 64-KiB region in XIP Flash is reserved for the kernel symbol catalog; only its length field and actual symbol records are used.
- `tools/build_sdk.sh` performs an initial link, extracts symbols with `nm`, relinks with the catalog, and checks that the final symbol addresses have not changed.
- User program symbols use slot-relative offsets; kernel symbols use absolute addresses. The separate interfaces are `/proc/sym/kernel` and `/proc/sym/userland/<program>`. The older combined symbol interface has been retired.
- Interactive memory viewing continues when symbol information is unavailable. Explicit `mem -s kernel` reports the problem and exits.

## Verification

```text
mem -s kernel
mem 0x20000000
mem 0x10000000
mem -s sh
```

**Verification note:** Post-link checks are distinct from runtime validation on hardware. Run build checks before flashing and verify `mem -s kernel` on the device when needed.
