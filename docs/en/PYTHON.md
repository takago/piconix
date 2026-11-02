# PicoNIX Tiny Python — User Guide

[日本語](../ja/PYTHON.md) | [Devices](DEVICES.md) | [I²C](I2C.md)

PicoNIX provides a small Python-like interpreter, not CPython or MicroPython. This guide describes the current `userland/python.c`, including I²C, sequences, and generic file I/O; full Python compatibility is not intended.

## First steps

Start the interactive interpreter with `python` and try `print(1 << 3)` or `print("hello", 123)`. For a script, create `/tmp/test.py` using `edit` and run `python /tmp/test.py`. There is **no `python3` command**. See [Devices](DEVICES.md) for the LED matrix orientation and [I²C](I2C.md) for BH1750 wiring. This is a deliberately limited Python-like interpreter, not CPython.

## REPL and scripts

Use `python` for the REPL and `python /tmp/test.py` to run a script. Enter `exit()` to quit the REPL. Supported control flow includes `if`, `while`, and `for ... in range(...)`. Use space indentation; submit a blank line to finish a REPL block. Both scripts and the REPL support multi-line lists and `bytes([...])` until brackets are closed.

## Integers, sequences, and strings

- Signed 32-bit integers support decimal (`123`), hexadecimal (`0x7B`), binary (`0b01111011`), and signed literals (`-0x20`). Uppercase prefixes also work.
- `a = [1, 2, 3]` creates a mutable integer list; `a[0]` reads and `a[1] = 5` updates an element.
- `b = bytes([0x01, 0xFF])` creates an immutable byte sequence, with values 0–255.
- `len(a)`, `len(b)`, `print(a)`, and `print(b)` work. Maximum sequence length: 64.
- Arrays and strings use a shared 1024-byte pool (256 32-bit words). Each sequence remains limited to 64 elements, and at most 16 variables can exist including integer scalars. Integer scalars do not consume pool space. Strings also use the pool, so the full 256-element capacity assumes no strings are stored. Pool exhaustion produces an error.
- String variables such as `message = "hello"` are limited to 64 bytes. Assignment between string variables, `len(message)`, and `print(message)` are supported. String concatenation and string indexing are not supported.
- Both `/` and `//` perform integer division. Floats, dictionaries, slicing, `append()`, `import`, `def`, and exceptions are not supported.

## Bitwise operators

Integer expressions support `<<` (left shift), `>>` (right shift), `&` (AND), `^` (XOR), `|` (OR), and `~` (bitwise NOT). Precedence follows Python: unary operators, multiplication/division, addition/subtraction, shifts, AND, XOR, OR, then comparisons. Shift counts must be 0–31. Integers are signed 32-bit values; left shift retains the low 32 bits.

```python
print(1 << 7, 0xF0 & 0x0F, 0x12 | 0x01)
fb = [0,0,0,0,0,0,0,0]
fb[0] = 1 << 3
fb[1] = fb[1] | (1 << 4)
```

## Multiple print() arguments

`print()` accepts zero or more arguments, separates their representations by spaces, and appends a newline. Supported arguments include integer expressions, string literals and variables, sequence variables, and integer list literals.

```python
message = "hello"
print([0x13, 0x12], message, 123)
# [19, 18] hello 123
print()
```

This is bounded string support in a tiny interpreter, not full Python string semantics.

## File descriptor API

| Function | Behavior |
|---|---|
| `open("/path", flags)` | Returns an integer FD. Flags: 0=read, 1=write, 2=read/write in this environment |
| `read(fd, n)` | Reads up to 64 bytes into an immutable byte sequence; use in assignment |
| `write(fd, data)` | Writes a string literal or sequence variable; returns bytes written |
| `close(fd)` | Closes an FD |
| `ioctl(fd, cmd, arg)` | Integer-argument ioctl; cannot directly construct pointer-based structures |

Paths in `open()` and strings in `write()` must be literals. Use `data = read(fd, 8)` for reads. Syntax and runtime failures share an error message such as `python: syntax/runtime error at statement N`. `N` is a statement index, not a physical source line number; blank lines and comments are omitted and continued lines are merged.

### Before drawing with Python

Run `fbctl test` from the shell first and check the physical LEDs. `/proc/fb` reports framebuffer data, not whether the wiring is correct. Refer to [Devices](DEVICES.md) for orientation, current limiting, and the pin connections.

For `write(fd, pattern)`, **each element is an 8-bit row and exactly eight elements are required**. For example, `bytes([0x80,0,0,0,0,0,0,0])` selects only the leftmost pixel of R0. A framebuffer 1 means lit, although the corresponding cathode GPIO is driven LOW.

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

- Source buffer: 2048 bytes; up to 64 active lines, 16 variable slots, 64 elements per sequence. REPL line buffer: 128 bytes.
- Sequence elements use 32-bit storage internally, including `bytes`.
- `print()` accepts multiple arguments. `input()` returns a string (up to 64 bytes); use `int(input())` for integer input. `msleep()` accepts 0–60000 ms.
- Hardware operation has been confirmed for BH1750 I²C readings, framebuffer writes and `FB_ENABLE`, and multi-line `bytes([...])` in scripts and the REPL. This is not exhaustive testing of all features.
- Former GPIO-specific Python helpers are replaced by the common file API and `/dev/gpio/<pin>`, `/dev/gpio/<pin>.dir`, `/dev/gpio/<pin>.pull`.

## String input and conversions

`input()` returns a string (up to 64 bytes). The optional prompt may be a string literal or a string variable. `int()` converts decimal strings to 32-bit integers and `str()` converts integers to strings. Invalid integer conversions cause a runtime error.

```python
name = input("Name: ")
age = int(input("Age: "))
print("Hello", name, age)
text = str(age)
print(text, len(name))
```

An empty input line produces an empty string. The existing 16-variable limit remains. String concatenation and general string expressions are unsupported. Replace old integer input `n = input()` with `n = int(input())`.

## Example: light-controlled LED wave

A BH1750 illuminance sensor controls the speed of a wave on the 8×8 LED matrix: brighter light makes the wave move faster and darkness slows it down. This example combines I²C input, integer and bitwise operations, and framebuffer output. **Tested on hardware.**

Connect the BH1750 to 3.3 V and GND, SDA to GP26, and SCL to GP27. Connect ADDR to GND to select I²C address `0x23`. See the [I²C guide](I2C.md) for wiring and bus details.

Save this code as `wave.py` and run `python wave.py` (or specify the saved path). It is **not** installed as an initial RAM file.

```python
# PicoNIX: light-controlled wave
wave = [3,4,5,6,5,4,3,2]
fb = [0,0,0,0,0,0,0,0]
fd = open("/dev/gpio/fb", 2)
ioctl(fd, FB_ENABLE, 0)
i2c_write(0x23, bytes([1]))
msleep(100)
i2c_write(0x23, bytes([0x10]))
msleep(200)

phase = 0
for n in range(10000):
    data = i2c_read(0x23, 2)
    lux = (data[0]*256+data[1])*5/6
    delay = 350 - lux/3
    if delay < 35:
        delay = 35
    for y in range(8):
        x = wave[(y+phase)%8]
        fb[y] = 1 << (7-x)
    write(fd, fb)
    phase = (phase+1)%8
    msleep(delay)

i2c_write(0x23, bytes([0]))
close(fd)
```

The `wave` array specifies the lit column in each row, and `phase` advances the pattern. The delay is computed as `350 - lux/3` milliseconds, clamped to a minimum of 35 ms. After about 10,000 frames, the script powers down the BH1750 and exits. Division in PicoNIX Tiny Python is integer division. This requires an external sensor and does not show numeric lux values on the display.

