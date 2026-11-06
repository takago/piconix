# PicoNIX C I2C examples

These are **C user-space source examples**, not firmware-bundled commands. All access `/dev/i2c/1` through `OS_I2C_TRANSFER` (`GP26` SDA, `GP27` SCL). The devices need **3.3 V-compatible** I2C wiring and a shared ground. Check module voltage, pull-ups and address before connecting.

| File | Source program | Result |
|---|---|---|
| `adt7410.c` | `adt7410.py`, `adt7410_tk.py` | Temperature in Celsius (default 13-bit mode) |
| `bh1750.c` | `bh1750.py`, `bh1750_tk.py` | Lux, continuous high-resolution mode |
| `bme280.c` | `bme280.py` | Temperature, pressure, humidity, integer compensation |
| `bmx055_acc.c` | `bmx055_acc.py` | Raw 3-axis acceleration (no roll/pitch math) |
| `bmx055_gyro.c` | `bmx055_gyro.py` | Raw 3-axis gyro readings |
| `bmx055_mag.c` | `bmx055_mag.py` | Raw 3-axis magnetometer readings (not compensated) |
| `lcd0802.c` | `lcd0802.py` | Two-line LCD output |
| `rpr0521rs.c` | `rpr0521rs.py` | Raw proximity and ALS channels |

Tkinter display programs are represented by terminal-output versions; PicoNIX has no Tkinter. These examples deliberately keep each sensor program separate and share only the small `sensor_i2c.h` helper. They do not modify the kernel or interpreter.

## Integrating a C example

1. Copy the desired `.c` and `sensor_i2c.h` into `userland/apps/`.
2. **Adjust the header include** in the copied `.c` to `#include "sensor_i2c.h"`, The shared header uses `#include "piconix_i2c.h"`, resolved through the existing build include path.
3. Add the command name to the application's CMake list in `CMakeLists.txt`, following existing apps. Rebuild with the existing PicoNIX build procedure, then flash the firmware.
4. Run the command from the PicoNIX shell. The sample loops run until terminated; the LCD sample runs once.

**Important:** These are source-level ports, not yet verified on a physical sensor or with the full PicoNIX firmware build. In particular, BME280's 64-bit integer compensation must be tested for available runtime helpers and code size. The BME280 address is `0x76` (some boards use `0x77`). The original accelerometer's `atan2`/`sqrt` roll/pitch computation is omitted; only raw readings are displayed. Magnetometer data is uncalibrated. Existing `userland/apps/bh1750.c` already provides a BH1750 command; do not add a second app with the same name.

## I2C helper naming

The shared `sensor_i2c.h` defines `i2c_open()`, `i2c_xfer()`, `i2c_read_reg()`, and `i2c_write_reg()`. `i2c_xfer()` matches the name and combined-transfer semantics of PicoNIX Python `i2c_xfer()`, but C uses separate input/output buffers and lengths. `i2c_read_reg()` and `i2c_write_reg()` are convenience wrappers, not kernel or C-library APIs. `i2c_open()` opens `/dev/i2c/1`; close the returned descriptor with `close()`. These are `static` example-local helpers and do not change the public ABI.
