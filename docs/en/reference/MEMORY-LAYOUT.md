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

`/dev/shm` exposes a common 2-KiB byte-addressed RAM area with `read`, `write` and `lseek` (SEEK_SET / SEEK_CUR / SEEK_END). PIC applications can access the same physical region directly. SHM is cleared on boot.

The `mem` viewer labels the relocated regions, and the `trace` ring is at the new 16-KiB address.

**Validation status:** Pico W ELF layout audit passed. Hardware checks confirmed `/proc/memory`, `/proc/ramfiles`, `/dev/shm`, `/dev/mem`, `mem`, and `trace`. Network communication has also been reported working, but exhaustive AP/STA stability testing is not established.

## Pico W network bank

The 16-KiB network reserve is divided into the 10-KiB lwIP heap
(`0x2003C000..0x2003E7FF`), a 2-KiB boundary gap
(`0x2003E800..0x2003EFFF`), and a 4-KiB idle/network task stack
(`0x2003F000..0x2003FFFF`). The gap includes lwIP's heap allocator metadata;
it is not a free 2-KiB allocation pool. The ordinary Pico still places its
idle stack in kernel SRAM. lwIP's static pbuf pools, PCB pools and CYW43
state remain in kernel SRAM; the relocated idle stack frees 4 KiB there.
Use `cat /proc/net/memory` for heap and pool peak usage and allocation errors.
The fixed-address layout must be verified after linking on Pico W.

## Shared-memory assembly example

The first four bytes of `/dev/shm` can serve as a shared 32-bit counter. This example accesses physical address `0x20032000` directly and increments that counter. The value survives process exit and subsequent executions, but resets to zero when the system reboots.

```asm
@ Increment shared 32-bit counter at physical address 0x20032000.
@ Run repeatedly; each process sees the same shared memory.
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

Save the program as `/tmp/counter.s`, then run:

```sh
as counter.s
./a.out
./a.out
./a.out
dd if=/dev/shm of=- bs=1 count=4 | hexdump
```

If the counter was initially zero, three runs produce the bytes `03 00 00 00` in little-endian order. A 16-bit-oriented `hexdump` may instead show `0003 0000`. The assembler writes `/tmp/a.out`.

**Caution:** The `ldr`/increment/`str` sequence is not atomic. Concurrent processes can lose updates. Run the example sequentially; `/dev/shm` is shared with other programs, so do not overwrite the first four bytes when they are in use.
