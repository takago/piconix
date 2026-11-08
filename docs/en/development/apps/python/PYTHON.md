# PicoNIX Tiny Python — User Guide

**I/O convention (2026-11-05):** `open()`, `socket()` and `accept()` return an FD or `-1`. `read()`, `recv()`, `recvfrom()`, `i2c_read()` and `i2c_xfer()` return byte arrays on success or integer `-1` on failure. Failed `write()`, `send()`, `sendto()`, `i2c_write()`, `close()`, `ioctl()`, `connect()`, `bind()`, `listen()` and `lseek()` return `-1`. Nonblocking `EAGAIN` is `-1`, whereas TCP EOF or a successful zero-byte result is `[]`. Check `data == -1` before calling `len(data)`. Successful `status()` values are preserved.


*Development guide*

## Before you start

Start with interactive calculations and printing, then save and run a short script in `/tmp`. Standard Python `import` and floating-point operations are unavailable.


[日本語](../../../../ja/development/apps/python/PYTHON.md) | [Devices](../../../reference/devices/DEVICES.md) | [I²C](../../../reference/devices/I2C.md)

PicoNIX provides a small Python-like interpreter, not CPython or MicroPython. This guide describes the current `userland/apps/python.c`, including I²C, sequences, and generic file I/O; full Python compatibility is not intended.

**Hands-on guides:** [Device files, GPIO, ADC, PWM, LED and I²C](DEVICE-PROGRAMMING.md) / [TCP/UDP sockets (Pico W)](SOCKET-PROGRAMMING.md). This page covers the syntax and API overview; the linked guides contain step-by-step exercises.

## First steps

Start the interactive interpreter with `python` and try `print(1 << 3)` or `print("hello", 123)`. For a script, create `/tmp/test.py` using `edit` and run `python /tmp/test.py`. There is **no `python3` command**. The preinstalled `/tmp/test.py` reads the RP2040 on-chip temperature sensor at `/dev/adc/temp` five times at one-second intervals, without networking. Since `read()` returns a byte sequence, it displays each reading with `write(1, temp)` and reopens the device for a fresh measurement.

See [hardware wiring](../../../reference/hardware/WIRING.md) for LED matrix orientation and BH1750 connections; see [Devices](../../../reference/devices/DEVICES.md) and [I²C](../../../reference/devices/I2C.md) for the interfaces. This is a deliberately limited Python-like interpreter, not CPython.

## REPL and scripts

Use `python` for the REPL and `python /tmp/test.py` to run a script. `exit()` terminates either the REPL or a script successfully, including from inside an `if` or loop. `True` and `False` are represented as integers `1` and `0`; there is no separate `bool` type. Supported control flow includes `if`, `while`, and `for ... in range(...)`. Use space indentation; submit a blank line to finish a REPL block. Both scripts and the REPL support multi-line lists and `bytes([...])` until brackets are closed.

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
| `read(fd, n)` | Returns up to 64 bytes as an immutable sequence, or `-1` on failure; use in assignment |
| `write(fd, data)` | Writes a string literal or sequence variable; returns bytes written |
| `close(fd)` | Closes an FD |
| `ioctl(fd, cmd, arg)` | Integer-argument ioctl; cannot directly construct pointer-based structures |

Paths in `open()` and strings in `write()` must be literals. Use `data = read(fd, 8)` for reads. Operation failures return `-1`; syntax and invalid arguments share an error message such as `python: syntax/runtime error at statement N`. `N` is a statement index, not a physical source line number; blank lines and comments are omitted and continued lines are merged.

### Before drawing with Python

Run `fbctl test` from the shell first and check the physical LEDs. `/proc/fb` reports framebuffer data, not whether the wiring is correct. Refer to [hardware wiring](../../../reference/hardware/WIRING.md) for orientation, current limiting, and the pin connections; [Devices](../../../reference/devices/DEVICES.md) covers the framebuffer interface.

For `write(fd, pattern)`, **each element is an 8-bit row and exactly eight elements are required**. For example, `bytes([0x80,0,0,0,0,0,0,0])` selects only the leftmost pixel of R0. A framebuffer 1 means lit, although the corresponding cathode GPIO is driven LOW.

### LED matrix

```python
pattern = bytes([0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c])
fd = open("/dev/fb", 2)
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

Use `i2c_write(fd, addr, sequence)`, `data = i2c_read(fd, addr, count)`, or `data = i2c_xfer(fd, addr, sequence, count)`. Valid addresses are 0x08–0x77, and each phase is limited to 32 bytes. `i2c_xfer()` uses repeated START. `i2c_write()` returns the number of bytes written, or `-1` on transfer failure. `i2c_read()` and `i2c_xfer()` return a byte list, or the integer `-1` on failure. Use `if data != -1:` to test the result. `open()` also returns `-1` if opening fails. `read()`, `recv()`, and `recvfrom()` return a byte sequence on success or `-1` on operation failure. An empty sequence represents a zero-length result. `EAGAIN` also becomes `-1`, so pending data cannot be distinguished from other failures. Invalid syntax or arguments still cause an error. Inline payloads such as `i2c_write(i2c, 0x23, bytes([0x10]))` are supported. Both `i2c_write()` and `i2c_xfer()` accept either inline `bytes([0x00])` or a byte-array variable as the write payload; for example, `data = i2c_xfer(fd, 0x48, bytes([0x00]), 2)`. Values must be 0–255 and each write is limited to 32 bytes. Read operations still return a byte array or `-1`; assign the result before checking it. `i2c_write()` is an integer-valued expression and may be used in conditions such as `if i2c_write(fd, 0x23, bytes([0x10])) < 0:`, assignments, or standalone calls. Use these helpers for the pointer-based transfer request. Open the bus once with `fd = open("/dev/i2c/1", 2)`, reuse that FD for transfers, and `close(fd)` when finished. The FD argument is required; the previous two-argument `i2c_write()` form is no longer supported.

### BH1750 (address 0x23)

```python
i2c = open("/dev/i2c/1", 2)
power = bytes([0x01])
i2c_write(i2c, 0x23, power)
msleep(100)
mode = bytes([0x10])
i2c_write(i2c, 0x23, mode)
msleep(200)
for i in range(10):
    data = i2c_read(i2c, 0x23, 2)
    if data == -1:
        break
    raw = data[0] * 256 + data[1]
    lux1000 = raw * 1000 * 5 / 6
    print(lux1000)
    msleep(200)
off = bytes([0x00])
i2c_write(i2c, 0x23, off)
close(i2c)
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

A BH1750 illuminance sensor controls the speed of a wave on the 8×8 LED matrix: brighter light makes the wave move faster and darkness slows it down. This example combines I²C input, integer and bitwise operations, and framebuffer output. The previous sensor-driven version was tested on hardware; this fallback variant has not yet been tested on a device.

Connect the BH1750 to 3.3 V and GND, SDA to GP26, and SCL to GP27. Connect ADDR to GND to select I²C address `0x23`. See [hardware wiring](../../../reference/hardware/WIRING.md) for electrical connections and the [I²C guide](../../../reference/devices/I2C.md) for the bus interface.

The sample source is [`userland/examples/python/fbwave.py`](../../../../../userland/examples/python/fbwave.py). The preinstalled copy is `userland/fs/tmp_common/fbwave.py`, exposed as `/tmp/fbwave.py` at boot. Run `python /tmp/fbwave.py`.

```python
# PicoNIX: light-controlled wave (fixed speed without BH1750)
wave = [3,4,5,6,5,4,3,2]
fb = [0,0,0,0,0,0,0,0]
fd = open("/dev/fb", 2)
if fd >= 0:
    ioctl(fd, FB_ENABLE, 0)
i2c = open("/dev/i2c/1", 2)
ready = 0
if i2c >= 0:
    ready = i2c_write(i2c, 0x23, bytes([1]))
    if ready >= 0:
        ready = 1
    if ready > 0:
        msleep(100)
        ready = i2c_write(i2c, 0x23, bytes([0x10]))
        if ready >= 0:
            ready = 1
        if ready > 0:
            msleep(200)

phase = 0
for n in range(10000):
    delay = 150
    if ready > 0:
        data = i2c_read(i2c, 0x23, 2)
        if data != -1:
            lux = (data[0]*256+data[1])*5/6
            delay = 350 - lux/3
            if delay < 35:
                delay = 35
        else:
            ready = 0
    for y in range(8):
        x = wave[(y+phase)%8]
        fb[y] = 1 << (7-x)
    if fd >= 0:
        write(fd, fb)
    phase = (phase+1)%8
    msleep(delay)

if i2c >= 0:
    if ready > 0:
        off = i2c_write(i2c, 0x23, bytes([0]))
    close(i2c)
if fd >= 0:
    close(fd)
```

The `wave` array specifies the lit column in each row, and `phase` advances the pattern. The delay is computed as `350 - lux/3` milliseconds, clamped to a minimum of 35 ms. If the I²C device cannot be opened, initialization writes fail, or a sensor read fails, the wave continues at a fixed 150 ms per frame. After a read failure, it does not retry sensor access. After about 10,000 frames, the script powers down the BH1750 if available and exits. Division in PicoNIX Tiny Python is integer division. The wave also runs without a sensor, at fixed speed, and does not show numeric lux values on the display.


## Socket communication (Pico W)

This is a PicoNIX-specific integer-FD API, not CPython's `import socket`. It uses numeric IPv4 addresses. Sockets are blocking by default. `accept()`, `recv()`, and `recvfrom()` suspend the task until a connection or data arrives. Built-in Python also supports `fcntl(fd, F_GETFL, 0)` and `fcntl(fd, F_SETFL, flags | O_NONBLOCK)` for switching supported socket descriptors to nonblocking mode.

| API | Behavior |
|---|---|
| `socket(1)` / `socket(2)` | Create a TCP / UDP socket |
| `connect(fd, "127.0.0.1", 80)`, `status(fd)` | Start TCP connection / poll status (0=pending, 1=ready) |
| `listen(fd, port)`, `accept(fd)` | TCP listener / accept (-11 when no client) |
| `bind(fd, port)` | Bind UDP receive port |
| `send(fd, data)`, `write(fd, data)` | TCP send; may be partial |
| `data = recv(fd, n)` | TCP receive, up to 64 bytes; pending/errors return `-1`; EOF returns `[]` |
| `sendto(fd, data, "192.168.4.2", port)` | UDP send |
| `data = recvfrom(fd, n)` | UDP receive, up to 64 bytes; `-1` if pending/error |
| `peerport()` | Source port of the most recent `recvfrom()` |
| `sendto(fd, data, "peer", 0)` | Reply to the most recent UDP sender (`peer` / 0 are special syntax) |

`data` may be a string literal, string variable, list/bytes variable, or `str(integer_expression)` (maximum 64 bytes). String literals support `\n`, `\r`, `\t`, `\\`, `\"`, `\'`, and `\xHH`. NUL is not supported in ordinary stored strings, but `\0` in a send literal transmits a zero byte.

```python
fd = socket(1)
if fd < 0:
    print("socket failed")
    exit()
if connect(fd, "127.0.0.1", 80) != 0:
    print("connect failed")
    close(fd)
    exit()
state = 0
while state == 0:
    state = status(fd)
    msleep(10)
if state != 1:
    print("connection failed")
    close(fd)
    exit()
send(fd, "GET / HTTP/1.0\r\n\r\n")
data = recv(fd, 64)
if data != -1:
    print(data)
close(fd)
```

### TCP and UDP temperature servers

Both Pico W examples use `open("/dev/adc/temp", 0)` and `read(sensor, 32)`, returning Celsius text such as `26.3\n`. The sensor is opened for each request. The RP2040 on-chip sensor approximates chip temperature, not room temperature.

| Example | Protocol | Behavior |
|---|---|---|
| `userland/examples/python/temp_tcp.py` | TCP 9000 | Send one reading upon connection, then close |
| `userland/examples/python/temp_udp.py` | UDP 9000 | Reply to each incoming request datagram |

Copy these scripts to `/tmp/temp_tcp.py` and `/tmp/temp_udp.py`, then run `python /tmp/temp_tcp.py` or `python /tmp/temp_udp.py` on Pico W.

On a host on the same LAN:

```bash
# TCP
nc <Pico W IP address> 9000

# UDP
printf 'temp' | nc -u -w 2 <Pico W IP address> 9000
```

TCP and UDP have separate port spaces. `accept()` and `recvfrom()` block by default, so the examples do not need polling delays such as `msleep()`. TCP `send()` may be partial or temporarily unavailable; the TCP sample makes a single short send without retries. UDP `"peer"` with port `0` addresses only the most recently received sender. Each Python `while` loop is limited to 10,000 iterations, so these examples are not permanent services. Use only on a trusted test LAN.

---

**Related links:** [Development guide index](../../README.md) | [Learning roadmap](../../../ROADMAP.md)

**Error handling:** Samples print an error and call `exit()` if a required device or socket cannot be initialized, closing open FDs where necessary. `exit()` terminates with status 0; it is not a substitute for `exit(1)` or exceptions. The optional BH1750 sensor falls back to fixed-speed animation when unavailable.

## I2C sensor examples

See the [Python I2C examples](../../../../../userland/examples/python/README.md) for ADT7410, BH1750, LCD0802 and RPR-0521RS. They use PicoNIX integer arithmetic and I2C APIs. Hardware validation is pending.

### `fcntl()` and nonblocking I/O

`fcntl(fd, cmd, value)` takes the same three arguments as the PicoNIX C API. Constants include `F_GETFL`, `F_SETFL`, and `O_NONBLOCK`. Only `O_NONBLOCK` can be changed via `F_SETFL`, and only on supported TTY and TCP/UDP socket descriptors.

```python
flags = fcntl(fd, F_GETFL, 0)
if flags >= 0:
    result = fcntl(fd, F_SETFL, flags | O_NONBLOCK)
```

On nonblocking sockets, `recv()`, `recvfrom()`, and `accept()` return `-1` when no data or connection is ready. TTY `read()` instead returns `[]` when no input is ready; semantics depend on the descriptor type. This is a PicoNIX built-in, not CPython fcntl module.
