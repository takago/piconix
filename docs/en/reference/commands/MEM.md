# mem — read-only memory viewer

*Usage guide*

`mem` is a full-screen hexadecimal viewer for PicoNIX SRAM and XIP Flash. **It cannot edit or overwrite memory.**

## Start with a memory view

```sh
mem
```

The display shows addresses and the bytes stored there, **16 bytes per row**. Roughly once per second, it rereads `/dev/mem` and highlights bytes that differ from the previous reading. Highlighting means the value changed; `mem` did not write it.

```sh
mem 0x20018000
```

Specify a starting address as an argument. Supported `/dev/mem` ranges are XIP Flash `0x10000000`–`0x101fffff` and SRAM `0x20000000`–`0x2003ffff`. Addresses outside those regions cannot be viewed.

## Navigate the display

| Key | Action |
|---|---|
| `↑` / `↓` | Move 16 bytes (one row) |
| `PgUp` / `PgDn` | Move by one visible page |
| `SPACE` | Pause or resume periodic updates |
| `1`–`6` | Jump to the corresponding 24-KiB user task slot |
| `k` | Jump to kernel RAM |
| `K` | Jump to XIP Flash |
| `g` | Enter a hexadecimal address |
| `q` | Quit |

The viewer reserves three header rows and three control rows, then uses the remaining height for data: up to 36 data rows (576 bytes). A 24-line terminal has 18 data rows (288 bytes). If the terminal size query fails, it assumes 80×24. Resizing the terminal while `mem` runs does not change its layout.

## Use symbols to interpret addresses

```sh
mem -s
mem -s kernel
mem -s sh
```

`mem -s` lists symbol sources; `mem -s kernel` lists kernel symbols, and `mem -s sh` lists shell symbols. The interactive viewer also shows symbols relevant to the current address. Symbols help you identify nearby functions and data; not every address has a symbol. Memory inspection remains available without symbol data.

**Symbols versus tracing:** `mem -s kernel` uses names and addresses generated from the linked ELF. The `TRACE_MATCH` list in `kernel/kernel.c` separately selects functions recorded by the kernel tracer; it does not use the ELF symbol list as its filter. See the [trace guide](TRACE.md).

## Notes for observation

- Running tasks may change SRAM concurrently. The display is not an atomic snapshot.
- User slots reached with `1`–`6` can change as tasks start and finish.
- Kernel RAM (`k`) and XIP Flash (`K`) serve different purposes; much kernel code executes directly from Flash.
- If values appear to stop updating, check whether `SPACE` paused refresh.


---

**Related links:** [Guide index](README.md) | [Trace](TRACE.md) | [日本語](../../../ja/reference/commands/MEM.md)
