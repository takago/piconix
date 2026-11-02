# dd — binary copying and RAM transfer

[日本語](../ja/DD.md) | [Documentation](DOCS.md) | [nc](NC.md) | [Devices](DEVICES.md)

`dd` transfers binary data between files, devices, and pipelines. Options: `if=`, `of=`, `bs=`, `count=`, `skip=`, and `seek=`. `if=-` means stdin and `of=-` means stdout; either FD is also selected if the option is omitted. `bs` is 1–256 bytes; `skip` and `seek` count blocks of size `bs`. Numeric arguments accept decimal and `0x` hexadecimal notation.

```sh
dd if=/tmp/test.py of=/tmp/copy.py bs=128
dd if=/dev/mem of=/tmp/dump.bin bs=1 skip=0x2000E000 count=256
hexdump /tmp/dump.bin
```

Offsets into `/dev/mem` are absolute addresses. With `bs=1`, `skip` and `seek` are byte addresses. A `/dev/mem` input requires `count=`. Writes are currently restricted to user-slot RAM `0x2000E000`–`0x20031FFF` and RAM-file payload addresses shown in `/proc/ramfiles`. Writing task memory can corrupt a running program; use a verified idle slot only.

```sh
# Restore only to a verified unused slot
dd if=/tmp/dump.bin of=/dev/mem bs=1 seek=0x2000E000 count=256
```

Raw writes to RAM-file payloads do not update filename or file-length metadata. To create or transfer ordinary files, use `of=/tmp/name`. RAM files now have eleven 2048-byte slots, with payloads fixed from `0x20032800`. The shared 2-KiB RAM is available via `/dev/shm` (relative offset) or `/dev/mem` (absolute address starting at `0x20032000`).

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

## Linux → PicoNIX: write RAM

```sh
# PicoNIX: replace the destination with a verified safe address
nc -N -l 9000 | dd if=- of=/dev/mem bs=1 seek=0x2000E000 count=256
```

```sh
# Linux: send exactly 256 bytes
head -c 256 data.bin | nc -N 192.168.4.1 9000
```

Check transfer length, EOF handling, and the destination slot before writing. `/dev/mem` does not allow unrestricted writes to all RAM. Wi-Fi transfers require Pico W.
