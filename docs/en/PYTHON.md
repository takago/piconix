# PicoNIX Tiny Python — User Guide

[日本語](../ja/PYTHON.md) | [Devices](DEVICES.md) | [I²C](I2C.md)

PicoNIX provides a small Python-like interpreter, not CPython or MicroPython. This guide describes the current `userland/python.c`, including I²C, sequences, and generic file I/O; full Python compatibility is not intended.

## REPL and scripts

Use `python` for the REPL and `python /tmp/test.py` to run a script. Enter `exit()` to quit the REPL. Supported control flow includes `if`, `while`, and `for ... in range(...)`. Use space indentation; submit a blank line to finish a REPL block. Both scripts and the REPL support multi-line lists and `bytes([...])` until brackets are closed.

## Integers and sequences

- Signed 32-bit integers support decimal (`123`), hexadecimal (`0x7B`), binary (`0b01111011`), and signed literals (`-0x20`). Uppercase prefixes also work.
- `a = [1, 2, 3]` creates a mutable integer list; `a[0]` reads and `a[1] = 5` updates an element.
- `b = bytes([0x01, 0xFF])` creates an immutable byte sequence, with values 0–255.
- `len(a)`, `len(b)`, `print(a)`, and `print(b)` work. Maximum sequence length: 64.
- Both `/` and `//` perform integer division. Floats, dictionaries, slicing, `append()`, `import`, `def`, and exceptions are not supported.

## File descriptor API

| Function | Behavior |
|---|---|
| `open("/path", flags)` | Returns an integer FD. Flags: 0=read, 1=write, 2=read/write in this environment |
| `read(fd, n)` | Reads up to 64 bytes into an immutable byte sequence; use in assignment |
| `write(fd, data)` | Writes a string literal or sequence variable; returns bytes written |
| `close(fd)` | Closes an FD |
| `ioctl(fd, cmd, arg)` | Integer-argument ioctl; cannot directly construct pointer-based structures |

Paths in `open()` and strings in `write()` must be literals. Use `data = read(fd, 8)` for reads. Syntax and runtime failures share an error message such as `python: syntax/runtime error at statement N`. `N` is a statement index, not a physical source line number; blank lines and comments are omitted and continued lines are merged.

### LED matrix

```python
pattern = bytes([0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c])
fd = open("/dev/gpio/fb", 2)
ioctl(fd, FB_ENABLE, 0)
write(fd, pattern)
close(fd)
```

The framebuffer requires exactly eight bytes per write. `FB_ENABLE` enables scanning; `FB_DISABLE` disables the display and releases GPIO; `FB_STATUS` queries its state. Closing the FD does not disable the display.

### PWM

```python
fd = open("/dev/pwm/1/freq", 1)
write(fd, "440")
close(fd)
fd = open("/dev/pwm/1/a", 1)
write(fd, "50")
close(fd)
```

Frequency is 8–100000 Hz and duty is an integer percentage 0–100, both as text.

## I²C1

Use `i2c_write(addr, sequence)`, `data = i2c_read(addr, count)`, or `data = i2c_xfer(addr, sequence, count)`. Valid addresses are 0x08–0x77, and each phase is limited to 32 bytes. `i2c_xfer()` uses repeated START. Inline payloads such as `i2c_write(0x23, bytes([0x10]))` are supported. Use these helpers for the pointer-based transfer request.

### BH1750 (address 0x23)

```python
power = bytes([0x01])
i2c_write(0x23, power)
msleep(100)
mode = bytes([0x10])
i2c_write(0x23, mode)
msleep(200)
for i in range(10):
    data = i2c_read(0x23, 2)
    raw = data[0] * 256 + data[1]
    lux1000 = raw * 1000 * 5 / 6
    print(lux1000)
    msleep(200)
off = bytes([0x00])
i2c_write(0x23, off)
```

Output is **lux multiplied by 1000** (for example, 125833 means about 125.833 lx). Beware of 32-bit integer overflow.

## Limits and validation

- Source buffer: 1024 bytes; up to 64 active lines, 16 variable slots, 64 elements per sequence. REPL line buffer: 128 bytes.
- Sequence elements use 32-bit storage internally, including `bytes`.
- `print()` accepts one argument. `input()` reads an integer. `msleep()` accepts 0–60000 ms.
- Hardware operation has been confirmed for BH1750 I²C readings, framebuffer writes and `FB_ENABLE`, and multi-line `bytes([...])` in scripts and the REPL. This is not exhaustive testing of all features.
- Former GPIO-specific Python helpers are replaced by the common file API and `/dev/gpio/<pin>`, `/dev/gpio/<pin>.dir`, `/dev/gpio/<pin>.pull`.
