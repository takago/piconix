# Device Programming with Tiny Python

*Development guide — Raspberry Pi Pico and Pico W*

[Python basics](PYTHON.md) | [Socket programming](SOCKET-PROGRAMMING.md) | [Device reference](../../../reference/devices/DEVICES.md) | [Wiring](../../../reference/hardware/WIRING.md) | [日本語](../../../../ja/development/apps/python/DEVICE-PROGRAMMING.md)

PicoNIX exposes devices as files under `/dev`. Python uses `open()`, `read()`, `write()` and `close()` rather than a separate GPIO module. This guide follows `userland/apps/python.c` and the current device reference.

## Preparation

In the shell, inspect `ls /dev`, `ls /dev/gpio` and `ls /dev/adc`. Create a script with `edit /tmp/demo.py` and run `python /tmp/demo.py`. Standard Python `import`, `def` and `try` are unavailable. RAM-file size and filename restrictions apply.

### File-descriptor API

| Form | Behavior and limits |
|---|---|
| `fd = open("/dev/...", 0)` | Read-only. Flags 0=read, 1=write, 2=read/write |
| `data = read(fd, 32)` | Read up to 64 bytes as an immutable byte sequence. **Assignment is required** |
| `n = write(fd, "123")` | Write text or a byte sequence; return the actual byte count |
| `close(fd)` | Close a descriptor |
| `value = ioctl(fd, FB_STATUS, 0)` | Device control with an integer argument |
| `pos = lseek(fd, 0, SEEK_SET)` | Only on seekable FDs; `SEEK_SET`, `SEEK_CUR`, `SEEK_END` |

The path passed to `open()` must be a **string literal**. Reads and writes accept up to 64 bytes per call. `write()` can accept a literal, a string variable, a list or `bytes` variable, or `str(integer_expression)`. These are bytes, not CPython file objects; there is no `decode()`. Use `write(1, data)` to print received text without a numeric byte-array representation. Depending on the operation, errors may produce negative integer results or stop the interpreter with a runtime error.

## 1. ADC and on-chip temperature

First check `cat /dev/adc/temp` in the shell. Save as `/tmp/temp.py`:

```python
for i in range(5):
    sensor = open("/dev/adc/temp", 0)
    data = read(sensor, 32)
    close(sensor)
    write(1, data)
    msleep(1000)
```

Run `python /tmp/temp.py` for five readings spaced approximately one second apart. This is an approximate RP2040 chip temperature, not ambient temperature. Reopening the device for each reading obtains a fresh sample. A similar initial example is `/tmp/test.py`. External ADC availability depends on pin configuration; the standard configuration exposes `/dev/adc/2`.

## 2. GPIO and LED

Check `ls /dev/gpio` first. For GP25, save this as `/tmp/led.py`:

```python
fd = open("/dev/gpio/25", 1)
for i in range(5):
    write(fd, "1")
    msleep(300)
    write(fd, "0")
    msleep(300)
close(fd)
```

This example applies to the original Pico where GP25 drives the board LED. On Pico W, GP25 is reserved for CYW43 and `/dev/gpio/25` is not available. The Pico W onboard LED is on `WL_GPIO0`. GPIO direction and pull configuration use `/dev/gpio/<pin>.dir` and `/dev/gpio/<pin>.pull`. Check the [pin map](../../../reference/hardware/PINS.md) and [GPIO reference](../../../reference/devices/DEVICES.md).

## 3. PWM

Confirm safe wiring before connecting a buzzer or other load. PWM slice 1 channel A is GP18 and B is GP19; they share the frequency.

```python
fd = open("/dev/pwm/1/freq", 1)
write(fd, "440")
close(fd)
duty = open("/dev/pwm/1/a", 1)
write(duty, "50")
msleep(500)
write(duty, "0")
close(duty)
```

Frequency is 8–100000 Hz and duty is 0–100%. Follow the electrical wiring and load limits.

## 4. 8×8 LED framebuffer

Run `fbctl test` in the shell first to verify wiring and orientation. Each byte is one row; write eight rows at once.

```python
pattern = bytes([0x3c, 0x42, 0xa5, 0x81, 0xa5, 0x99, 0x42, 0x3c])
fd = open("/dev/fb", 2)
ioctl(fd, FB_ENABLE, 0)
write(fd, pattern)
close(fd)
```

Bit 1 means illuminated and 0 means off. The display remains enabled after `close()`. Run `fbctl off` in the shell to release its GPIO pins. `FB_STATUS` and `FB_DISABLE` are also supported.

## 5. I²C1 and BH1750

GP26 is SDA and GP27 is SCL. With ADDR connected to ground, a BH1750 normally uses address `0x23`. Check the wiring and run `i2cctl scan` before testing.

```python
fd = open("/dev/i2c/1", 2)
if fd >= 0:
    ok = i2c_write(fd, 0x23, bytes([0x01]))
    if ok >= 0:
        msleep(100)
        ok = i2c_write(fd, 0x23, bytes([0x10]))
        if ok >= 0:
            msleep(200)
            data = i2c_read(fd, 0x23, 2)
            if data != -1:
                raw = data[0] * 256 + data[1]
                print(raw * 5 / 6)
    close(fd)
```

This displays an integer approximation of illuminance in lux. Address range: `0x08`–`0x77`; maximum transfer size: 32 bytes per direction. Use `i2c_xfer(fd, addr, sequence, count)` for combined writes and reads with repeated START. See the [I²C reference](../../../reference/devices/I2C.md).

## 6. Runtime cautions

- Script source is limited to 2048 bytes and 64 active lines; there are 16 variable slots and each sequence is limited to 64 elements.
- `open()` or `ioctl()` failures can terminate the interpreter. Check device availability first with shell tools such as `ls`, `cat`, and `fbctl`.
- Return outputs to a safe state when a control program finishes, for example by setting PWM duty to zero.
- Binary devices such as `/dev/mem` and `/dev/trace` have different constraints from ordinary textual device nodes; consult their specifications.
- Direct Python I/O does not remove pin conflicts between GPIO, PWM, ADC and I²C.

**Next:** [TCP/UDP networking on Pico W](SOCKET-PROGRAMMING.md) / [Python fundamentals](PYTHON.md)
