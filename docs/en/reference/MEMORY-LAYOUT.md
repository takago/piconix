# RP2040 Memory Layout (ROM, Flash, and SRAM)

*Technical reference — distinguish nonvolatile storage, volatile memory, reservations, and actual usage*

## ROM and Flash (nonvolatile storage)

The RP2040 contains **16 KiB of on-chip Boot ROM** for bootstrapping and related services. It does **not** store the PicoNIX firmware. Standard Pico and Pico W boards instead have **2 MiB of external QSPI Flash**, mapped into the RP2040 XIP (Execute in Place) address window. Do not confuse the internal Boot ROM with external Flash.

| Region | Inclusive address range | Capacity / role |
|---|---|---|
| On-chip Boot ROM | `0x00000000..0x00003FFF` | 16 KiB of RP2040 boot code; not used for storing PicoNIX |
| External Flash XIP mapping | `0x10000000..0x101FFFFF` | 2 MiB on standard Pico/Pico W boards; contains firmware and read-only data |
| `.boot2` | `0x10000000..0x100000FF` | 256-byte SDK-generated second-stage bootloader |
| Remaining XIP firmware | Following `.boot2` | Kernel/SDK code, constants, embedded apps, symbol catalogs, manuals, etc.; exact bounds depend on the build |

Except for `.boot2`, these are **not individually fixed Flash reservations**. The linker places ordinary `.text` and `.rodata` alongside the `.piconix_app_images` section for embedded programs and symbol catalogs and the `.manual` section for the ROM manual. **Kernel code executes via XIP**, while **user applications are copied as PIC images from Flash into SRAM slots at launch**. Initial RAM-file contents are embedded in Flash and populated into RAM at startup. Initial values for `.data` are likewise stored in Flash and copied into SRAM during startup.

- ROM manual: `manual/`, `tools/make_manual_rom.py`, `cmake/embed_manual.S.in`, `kernel/manual.h`
- Embedded programs and symbol catalogs: `CMakeLists.txt`, `cmake/embed_image.S.in`, `kernel/section_extra_post_text.incl`
- Flash layout and audits: `tools/audit_firmware.py`, `tools/size_ram.py`

The stated 2 MiB corresponds to the **audited and readable Flash range for this version** on standard Pico/Pico W boards, not the entire RP2040 XIP address window. `/dev/mem` and `mem` also restrict Flash inspection to `0x10000000..0x101FFFFF`. PicoNIX does not provide a conventional persistent writable Flash filesystem.

After building, `python3 tools/size_ram.py` reports allocated Flash totals and **link-map-based approximate attribution** to PicoNIX, embedded apps, ROM manual, symbol catalogs, SDK, lwIP, CYW43, and others. This is neither the full Flash capacity nor a strict guarantee of remaining writable capacity. Inspect `piconix.elf` and the linker map for actual section addresses.

## SRAM (volatile memory)

PicoNIX partitions the RP2040's **256 KiB main SRAM** (`0x20000000..0x2003FFFF`) as follows. The sizes below are **reserved capacities**, not ELF usage or runtime free memory. Pico and Pico W use the same main-SRAM boundaries, but network-reserve usage and idle-stack placement differ.

| Region | Address range (inclusive) | Reserved | Purpose |
|---|---|---:|---|
| Kernel | `0x20000000..0x2000DFFF` | 56 KiB | Kernel data, metadata, stacks, etc. |
| User slots | `0x2000E000..0x20031FFF` | 144 KiB | Six 24-KiB process slots |
| SHM | `0x20032000..0x200327FF` | 2 KiB | `/dev/shm` shared memory |
| RAM files | `0x20032800..0x20037FFF` | 22 KiB | Eleven 2-KiB file payloads |
| Trace | `0x20038000..0x2003BFFF` | 16 KiB | Trace event ring |
| Network reserve | `0x2003C000..0x2003FFFF` | 16 KiB | lwIP heap etc. on Pico W |

The RP2040 additionally has **8 KiB scratch SRAM** (`0x20040000..0x20041FFF`), separate from the main 256 KiB shown above. Kernel code generally executes from XIP Flash; the 56-KiB kernel region is not the size of all executable kernel code.

## User slots and stacks

Up to six user processes each receive a fixed 24-KiB slot. The **final 4 KiB are reserved for the stack**, leaving at most 20 KiB for code, initialized data, and BSS. The linker asserts `__bss_end <= __stack_bottom`. See `include/piconix.h` for sizes, `kernel/kernel.c` for task placement and launch, and `userland/lib/slot.ld` for image/stack boundaries. A 24-KiB slot is not a 24-KiB application-file-size limit.

## RAM files, shared memory, and trace

Files under `/tmp` and `/etc` share **eleven fixed 2-KiB slots**. The file metadata (name, length, use state) remains inside the Kernel region, while payloads reside in the RAM files region. `/proc/ramfiles` reports the mapping. Direct writes through `/dev/mem` do not change a file's recorded length.

`/dev/shm` exposes a 2-KiB shared byte area supporting `read()`, `write()`, and `lseek()` (`SEEK_SET`, `SEEK_CUR`, `SEEK_END`). Applications can also address physical location `0x20032000` directly. SHM is cleared at startup. The trace event ring occupies 16 KiB beginning at `0x20038000`.

## Pico versus Pico W

On Pico W, the network reserve is used as follows:

| Purpose | Address range | Capacity |
|---|---|---:|
| lwIP heap payload | `0x2003C000..0x2003E7FF` | 10 KiB |
| Boundary margin including allocator metadata | `0x2003E800..0x2003EFFF` | 2 KiB |
| Idle stack | `0x2003F000..0x2003FFFF` | 4 KiB |

The 2-KiB margin is not entirely freely allocatable memory. lwIP static pbuf/PCB pools and CYW43 state also consume SRAM in the Kernel region. On a regular Pico, the idle stack instead resides inside the Kernel region; do not assume the Pico W network uses apply to a regular Pico.

## Inspecting layout and usage

After building on the host, run:

```sh
python3 tools/size_ram.py
```

This reports fixed reserved capacities separately from ELF-allocated bytes, and reports each application's code, data, BSS, reserved stack, and remaining capacity. `tools/verify_layout.py` checks fixed addresses and overlap; `tools/audit_wifi_elf.py` and `tools/audit_firmware.py` provide Pico W ELF auditing. **Passing layout checks does not establish runtime stack safety or network stability.**

Inside PicoNIX, inspect:

```sh
cat /proc/memory
cat /proc/ramfiles
cat /proc/net/memory
mem
```

`/proc/net/memory` is intended for observing network memory on Pico W. See `kernel/net.c` for network reporting and `kernel/kernel.c` for memory and process information.

## Shared-memory assembly example

The following increments a 32-bit counter in the first four bytes of `/dev/shm`. All processes can refer to the same physical address; values survive process exit but reset at reboot.

```asm
@ Increment shared 32-bit counter at physical address 0x20032000.
        adr     r2, shm_addr
        ldr     r2, [r2]
        ldr     r3, [r2]
        adds    r3, #1
        str     r3, [r2]
        movs    r0, #3
        movs    r1, #0
        svc     #0
        .align  2
shm_addr:
        .word   0x20032000
```

Save as `/tmp/counter.s` and execute:

```sh
as counter.s
./a.out
./a.out
./a.out
dd if=/dev/shm of=- bs=1 count=4 | hexdump
```

If the initial value was zero, the first four bytes after three runs are `03 00 00 00` (little endian). The presentation depends on `hexdump`'s display unit. **Load, increment, and store are not atomic**: this is not safe for concurrent increments. Do not overwrite shared memory currently used for another purpose.

---

**Related links:** [Reference index](README.md) | [Documentation index](../../DOCS.md) | [日本語](../../ja/reference/MEMORY-LAYOUT.md)
