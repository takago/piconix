# Wiring and Electrical Precautions

*Technical reference*

**Before powering up:** RP2040 GPIO uses 3.3 V logic. Do not apply 5 V logic or RS-232 voltage directly to GPIO. Unplug USB power before changing connections, use common ground, and provide suitable current limiting and driver circuitry. This document describes logical wiring; it is not a fully rated circuit schematic.

## LED matrix polarity

| Pico GPIO | Matrix connection | Active level |
|---|---|---|
| GP0–GP7 | Column cathodes | LOW selects lit columns |
| GP8–GP15 | Row anodes | HIGH selects the scanned row |

One row anode may carry current for several LEDs, so **use suitable current-limiting resistors and a driver circuit**. Avoid driving the matrix directly from RP2040 GPIO. Framebuffer data and `ioctl()` commands are documented in the [device reference](../devices/README.md).

## LED matrix orientation and coordinates

This example uses an **MNA20SR092G rotated 90 degrees clockwise from the manufacturer's standard orientation**. The matrix manufacturer's rows/columns and PicoNIX display `R`/`C` use different conventions. Rotation does not change anode/cathode polarity.

```text
          GP7 GP6 GP5 GP4 GP3 GP2 GP1 GP0
           C0  C1  C2  C3  C4  C5  C6  C7
          +-------------------------------+
GP8  R0   |  .   .   .   .   .   .   .   . |
GP9  R1   |  .   .   .   .   .   .   .   . |
GP10 R2   |  .   .   .   .   .   .   .   . |
GP11 R3   |  .   .   .   .   .   .   .   . |
GP12 R4   |  .   .   .   .   .   .   .   . |
GP13 R5   |  .   .   .   .   .   .   .   . |
GP14 R6   |  .   .   .   .   .   .   .   . |
GP15 R7   |  .   .   .   .   .   .   .   . |
          +-------------------------------+
```

`fb[0]` through `fb[7]` correspond to `R0` through `R7`. Bit 7 is the leftmost pixel (`C0`/GP7), and bit 0 the rightmost (`C7`/GP0). GP8–15 are row anodes (selected row HIGH); GP0–7 are column cathodes (lit column LOW).

The **physical module pin numbers below are from the MNA20SR092G datasheet** and show the corresponding PicoNIX wiring example. Verify actual wiring and display orientation before powering the circuit. Do not apply these physical pin numbers to a different LED module.

| Pico GPIO | Pico physical pin | PicoNIX display | MNA20SR092G pin |
|---|---:|---|---:|
| GP0 | 1 | C7 | 9 |
| GP1 | 2 | C6 | 14 |
| GP2 | 4 | C5 | 8 |
| GP3 | 5 | C4 | 12 |
| GP4 | 6 | C3 | 1 |
| GP5 | 7 | C2 | 7 |
| GP6 | 9 | C1 | 2 |
| GP7 | 10 | C0 | 5 |
| GP8 | 11 | R0 | 13 |
| GP9 | 12 | R1 | 3 |
| GP10 | 14 | R2 | 4 |
| GP11 | 15 | R3 | 10 |
| GP12 | 16 | R4 | 6 |
| GP13 | 17 | R5 | 11 |
| GP14 | 19 | R6 | 15 |
| GP15 | 20 | R7 | 16 |


## GPIO output bits and lit pixels

For each scanned row, `fb_tick()` generates 16 output bits for GP15 through GP0. The upper byte selects the anode row (GP15–8); the lower byte is the **inverted** framebuffer pixel byte (GP7–0). For `fb[0] = 0x81`, the output during the R0 scan is:

```text
GP15                                GP0
 0000 0001 0111 1110  =  0x017E
 -------- --------
 GP15-8   GP7-0

          C0  C1  C2  C3  C4  C5  C6  C7
R0        #   .   .   .   .   .   .   #
R1        .   .   .   .   .   .   .   .
...
R7        .   .   .   .   .   .   .   .
```

GP8 is HIGH to select R0, while GP7 and GP0 are LOW to light the two end pixels. `#` means lit and `.` means off.

| GP15–GP0 output | Pixels lit at that instant |
|---|---|
| `0x0100` | All eight pixels on R0 |
| `0x017E` | Both ends of R0 |
| `0x01FE` | Rightmost pixel on R0 (C7) |
| `0x02FD` | Second pixel from right on R1 (C6) |
| `0x8000` | All eight pixels on R7 |

These are **instantaneous GPIO values for one scan row**, not values that drive all eight rows simultaneously. The kernel advances one row per millisecond, refreshing the full display at about 125 Hz. Applications normally write eight bytes to `/dev/fb` instead of driving GPIO directly. Use suitable current limiting and a driver circuit for the LED matrix.

### Pixel bits versus GPIO output bits

For example, `fb[0] = 0x80` requests **the leftmost pixel C0 on R0**. In the framebuffer, a 1 means lit; the cathode output for a lit column is LOW. During the R0 scan, the GPIO output is therefore `0x017F` (only GP8 HIGH among anodes and only GP7 LOW among cathodes). With `fb[0] = 0`, the output during R0 is `0x01FF`, leaving all pixels off.

```text
framebuffer      R0: 10000000  (= 0x80, leftmost pixel)
GPIO GP15..GP0:      00000001 01111111  (= 0x017F)
```

**Note:** These values explain what the kernel outputs *during scanning*; they are not instructions to write a raw 16-bit value to GPIO. Use `fbctl` or `/dev/fb` for display experiments.


## Default wiring overview

| Pins | Function | Notes |
|---|---|---|
| GP0–7 | 8×8 LED matrix columns (cathodes) | Active LOW |
| GP8–15 | 8×8 LED matrix rows (anodes) | Active HIGH |
| GP16/17 | Push buttons | To GND, internal pull-up |
| GP18/19 | PWM | Shared frequency |
| GP20/21 | UART1 TX/RX | 115200 baud, 8N1 |
| GP26/27 | I²C1 SDA/SCL | 100 kHz; pull up to 3.3 V |
| GP28 | ADC2 | 0–3.3 V input |

Use a common GND and 3.3 V logic for UART; do not connect RS-232 voltages directly. An LED matrix requires current-limiting resistors and suitable driver circuitry. Use `pins` to inspect the pin assignment.

## Pushbuttons (GP16, GP17)

Connect each button between its GPIO and GND, with internal pull-up enabled. A pressed button reads LOW. Examples include `kitesys`.

## Buzzer, servo, and PWM (GP18, GP19)

GP18 and GP19 use PWM slice 1 A/B and **share a frequency**. GP18 is used in buzzer examples; GP19 is used for servo control. Do not power high-current loads or servos directly from a GPIO pin. Use suitable external power/driver hardware with common GND, based on the part's rating. Simultaneous independent PWM frequencies are not possible on these pins.

## UART1 (GP20, GP21)

Connect Pico GP20 (UART1 TX, physical header pin 26) to the remote RX, GP21 (UART1 RX, physical pin 27) to remote TX, and share GND. Use **3.3 V logic** hardware; never attach RS-232 voltage directly. Default serial settings are 115200 bps, 8N1. See [UART1 reference](../devices/UART.md) for `/dev/ttyS1` and `getty`.

## I²C1 (GP26, GP27)

Connect GP26 (SDA, physical pin 31) and GP27 (SCL, physical pin 32) to the corresponding I²C signals and share GND. SDA and SCL require **appropriate external pull-ups to 3.3 V**. Avoid duplicate or overly strong pull-ups when a module already includes resistors. Do not connect 5 V pull-ups directly. Consider bus speed and wire length; do not rely on internal pull-ups alone.

For a typical BH1750 breakout, VCC goes to a compatible 3.3 V supply, GND to GND, and SDA/SCL to GP26/27; ADDR to GND selects the common `0x23` address. **Check the actual module voltage requirements** before powering it. See the [I²C1 guide](../devices/I2C.md) for operations.

## ADC2 (GP28)

GP28 (physical header pin 34) provides ADC2. Apply an analog voltage within the RP2040's valid range (typically 0–3.3 V logic supply range). GP26/27 are reserved for I²C1 rather than ADC0/1 in this configuration. See [Device reference](../devices/README.md) for reading ADC values.

## Initial functional checks

Only run the checks appropriate for connected, safely driven hardware:

```sh
pins
fbctl test
cat /proc/fb
fbctl off
i2cctl scan
```

Use the `fbctl` commands only with a suitable LED driver circuit, and `i2cctl scan` only for devices that can safely be probed. **`cat /proc/fb` shows stored framebuffer state, not proof of correct physical wiring.**

---

**Related links:** [Hardware index](README.md) | [40-pin diagram](PINS.md) | [日本語](../../../ja/reference/hardware/WIRING.md)
