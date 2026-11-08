# dd — binary copying and RAM transfer

*Usage guide*

`dd` transfers binary data between files, devices, and pipelines. **An `of=` destination can be overwritten**, so start with a new filename when practicing ordinary file copies. Options: `if=`, `of=`, `bs=`, `count=`, `skip=`, and `seek=`. `if=-` means stdin and `of=-` means stdout; either FD is also selected if the option is omitted. `bs` is 1–256 bytes; `skip` and `seek` count blocks of size `bs`. Numeric arguments accept decimal and `0x` hexadecimal notation.

## Start by copying an ordinary file

```sh
dd if=/tmp/test.py of=/tmp/copy.py bs=128
dd if=/dev/mem of=/tmp/dump.bin bs=1 skip=0x2000E000 count=256
hexdump /tmp/dump.bin
```

The first command copies `/tmp/test.py` to `/tmp/copy.py`; `bs=128` sets the transfer block size to 128 bytes. `hexdump` helps inspect the copied bytes. The second example reads device memory and deserves additional care; read the notes below before using it.

## Memory and device safety

Offsets into `/dev/mem` are absolute addresses. With `bs=1`, `skip` and `seek` are byte addresses. A `/dev/mem` input requires `count=`. Writes are restricted to user-slot RAM `0x2000E000`–`0x20031FFF`, shared RAM (SHM) `0x20032000`–`0x200327FF`, and RAM-file payload addresses shown in `/proc/ramfiles`. Writing task memory can corrupt a running program; use a verified idle slot only.

```sh
# Safer exercise: write 256 bytes to shared RAM and read them back
dd if=/tmp/test.py of=/dev/shm bs=1 count=256
dd if=/dev/shm of=/tmp/shm.bin bs=1 count=256
```

Raw writes to RAM-file payloads do not update filename or file-length metadata. To create or transfer ordinary files, use `of=/tmp/name`. RAM files now have eleven 2048-byte slots, with payloads fixed from `0x20032800`. The shared 2-KiB RAM is available via `/dev/shm` (relative offset) or `/dev/mem` (absolute address starting at `0x20032000`).

## Before network transfers

The following `nc` examples require a **network-connected Pico W**. Check the actual IP addresses on both ends and start the receiving side first. RAM files also have size limits.

## Linux → PicoNIX: transfer a file

```sh
# PicoNIX: listen first
nc -N -l 9000 | dd if=- of=/tmp/data.bin bs=1
```

```sh
# Linux: use the real Pico W IP address
nc -N 192.168.4.1 9000 < data.bin
```

```sh
# PicoNIX: verify
ls -l /tmp/data.bin
hexdump /tmp/data.bin
```

The Linux `nc -N` option is implementation-specific; consult your installed netcat if the connection does not close as expected.

## PicoNIX → Linux: dump RAM

```sh
# Linux: listen first
nc -l 9000 > dump.bin
```

```sh
# PicoNIX: send 256 bytes; substitute Linux IP
dd if=/dev/mem bs=1 skip=0x2000E000 count=256 | nc -N 192.168.4.2 9000
```

```sh
# Linux: inspect
wc -c dump.bin
hexdump -C dump.bin
```

## Linux → PicoNIX: write shared RAM

```sh
# PicoNIX: receive up to 256 bytes into the beginning of shared RAM
nc -N -l 9000 | dd if=- of=/dev/shm bs=1 count=256
```

```sh
# Linux: send exactly 256 bytes
head -c 256 data.bin | nc -N 192.168.4.1 9000
```

Check transfer length and EOF handling. `/dev/shm` is shared by all tasks: do not overwrite data used by another program. Direct writes to task slots are not recommended as a normal learning exercise. `/dev/mem` does not allow unrestricted writes to all RAM. Wi-Fi transfers require Pico W.

## Troubleshooting transfers

If a transfer does not finish, check connection shutdown behavior and `-N` support on both versions of `nc`. For unexpected output, verify `count=`, `bs=`, and the sent and received byte counts. For `/dev/mem` read failures, check the allowed address range and that `count=` is specified.

---

**Related links:** [Pipe guide](PIPE.md) | [Guide index](README.md) | [日本語](../../../ja/reference/commands/DD.md) | [Documentation](../../README.md) | [nc](../network/NC.md) | [Devices](../devices/DEVICES.md)
