# PicoNIX Tiny Python I2C examples

These examples run with the PicoNIX `python` command, **not** CPython or pigpio. They are adapted from Raspberry Pi 3 pigpio examples. The Tkinter variants, BME280 and all BMX055 examples are intentionally excluded.

| Script | Device | Address | Output |
|---|---|---|---|
| `adt7410.py` | ADT7410 | `0x48` | Temperature in tenths of a degree Celsius (integer) |
| `bh1750.py` | BH1750 | `0x23` | Illuminance (integer lux) |
| `lcd0802.py` | I2C LCD (specific 0x3e controller) | `0x3e` | `Hello` / `(o_o)` |
| `rpr0521rs.py` | RPR-0521RS | `0x38` | Raw proximity and two ambient-light channels |

Connect the sensor to PicoNIX's I2C1 bus (see the hardware wiring guide), with correct supply voltage, common ground and pull-ups as required by the board. The LCD initialization sequence is **controller-specific**, not universal to all I2C LCDs.

Copy one script to the PicoNIX RAM filesystem, e.g. `/tmp/adt7410.py`, then run `python /tmp/adt7410.py`. The samples use `open("/dev/i2c/1", 2)`, `i2c_write`, `i2c_read`, and `i2c_xfer`; they do not require changes to the interpreter. Each measuring example runs at most 10,000 iterations, matching the interpreter's loop limit. `exit()` is used for initialization failures. Stop a long-running sample using the terminal's process control.

The ADT7410 conversion assumes its default 13-bit resolution configuration; it prints tenths of a degree without floating point. BH1750 uses the standard 1.2 divisor, expressed as integer `*5/6`. RPR-0521RS outputs raw channels, not calibrated lux. No hardware testing has been performed.
