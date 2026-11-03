# PicoNIX Device Guide

[日本語](../ja/DEVICES.md) | [README](README.md)

For the full directory and node inventory, access modes, and RAM file limits, see [Filesystem and device nodes](FILESYSTEM.md).

PicoNIX exposes hardware through virtual files under `/dev`. These are not stored on disk: the kernel routes `open()`, `read()`, and `write()` to the device implementations.

## Device overview

| Path | Purpose |
|---|---|
| `/dev/console` | Main console (USB CDC) |
| `/dev/ttyS1` | UART1 terminal |
| `/dev/ttyN0` | Network terminal in Wi-Fi builds (see [NETWORK.md](NETWORK.md)) |
| `/dev/net/wifi` | STA control (`echo restart > /dev/net/wifi`); not available in non-Wi-Fi builds, AP mode rejects restart |
| `/dev/rand` | Non-cryptographic pseudorandom bytes (read-only; at most 4 bytes per `read()` call) |
| `/dev/gpio/fb` | 64-bit framebuffer for an 8×8 LED matrix |
| `/dev/gpio/` | Pins configured for GPIO in `pins.h` |
| `/dev/adc/` | Enabled ADC inputs and on-chip temperature sensor |
| `/dev/pwm/` | Enabled PWM outputs and common frequency |
| `/dev/i2c/1` | I²C1 master (GP26 SDA, GP27 SCL) |
| `/dev/mem` | Read permitted RAM/flash; restricted writes to task slots, SHM and RAM-file payloads |
| `/dev/shm` | Common 2 KiB of shared RAM; supports `read()`, `write()`, `lseek()` |
| `/dev/trace` | Binary trace records |

## 8×8 LED framebuffer

PicoNIX provides a monochrome 8×8 LED-matrix framebuffer connected to GP0–15. Its device path is **`/dev/gpio/fb`**, not `/dev/fb` or `/dev/fb0`. The default wiring assumes the main console uses USB CDC.

### Data layout and wiring

The framebuffer holds **64 bits (8 bytes)**: one byte per row. `buffer[0]` through `buffer[7]` correspond to GP8 through GP15 (row anodes). Bits 0–7 in each byte correspond to GP0–GP7 (column cathodes). A set bit turns its LED **on**. In the text view provided by `/proc/fb`, bit 7 appears at the left and bit 0 at the right.

| Pins | Matrix connection | Active level |
|---|---|---|
| GP0–7 | Column cathodes | LOW selects illuminated columns |
| GP8–15 | Row anodes | HIGH selects the scanned row |

A SysTick handler scans **one row every 1 ms**, completing a frame in approximately 8 ms (**125 Hz**). There is no brightness control. Anodes are blanked before each row change to reduce ghosting.

### Default pinout (Pico / Pico W)

The 40-pin header below is viewed from the **component side with USB at the top**. Numbers at either side are physical Pico pin numbers; `R`/`C` refer to PicoNIX LED display coordinates. `kernel/pins.h` is the source of truth for configured functions.

```text
                      USB
              +-------------------+
       C7  1 |GP0          VBUS| 40 
       C6  2 |GP1          VSYS| 39 
           3 |GND           GND| 38 
       C5  4 |GP2        3V3_EN| 37 
       C4  5 |GP3           3V3| 36 
       C3  6 |GP4      ADC_VREF| 35 
       C2  7 |GP5          GP28| 34 ADC2
           8 |GND           GND| 33 
       C1  9 |GP6          GP27| 32 I2C1 SCL
       C0 10 |GP7          GP26| 31 I2C1 SDA
       R0 11 |GP8           RUN| 30 
       R1 12 |GP9          GP22| 29 GPIO
          13 |GND           GND| 28 
       R2 14 |GP10         GP21| 27 UART1 RX
       R3 15 |GP11         GP20| 26 UART1 TX
       R4 16 |GP12         GP19| 25 PWM B
       R5 17 |GP13         GP18| 24 PWM A
          18 |GND           GND| 23 
       R6 19 |GP14         GP17| 22 Switch 2
       R7 20 |GP15         GP16| 21 Switch 1
              +-------------------+
```

On Pico W, GP23/24 are used for onboard wireless-related functions and are not general external wiring pins. GP25 is not exposed on the 40-pin header; its onboard LED usage differs between Pico and Pico W.

### LED matrix orientation and coordinates

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


### GPIO output bits and lit pixels

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

These are **instantaneous GPIO values for one scan row**, not values that drive all eight rows simultaneously. The kernel advances one row per millisecond, refreshing the full display at about 125 Hz. Applications normally write eight bytes to `/dev/gpio/fb` instead of driving GPIO directly. Use suitable current limiting and a driver circuit for the LED matrix.

### First display: a quick check

Verify the wiring, current limiting, and driver circuitry before running these commands. The Pico and Pico W share the 40-pin header positions, but onboard functions such as GP25 differ.

```sh
fbctl test
cat /proc/fb
fbctl off
```

The first command displays a heart. The second prints the eight stored pixel rows; the third stops scanning and releases GP0–15 for GPIO use. **`cat /proc/fb` shows stored bytes, not proof that the physical LEDs are wired correctly.** Check the actual display visually.

#### Pixel bits versus GPIO output bits

For example, `fb[0] = 0x80` requests **the leftmost pixel C0 on R0**. In the framebuffer, a 1 means lit; the cathode output for a lit column is LOW. During the R0 scan, the GPIO output is therefore `0x017F` (only GP8 HIGH among anodes and only GP7 LOW among cathodes). With `fb[0] = 0`, the output during R0 is `0x01FF`, leaving all pixels off.

```text
framebuffer      R0: 10000000  (= 0x80, leftmost pixel)
GPIO GP15..GP0:      00000001 01111111  (= 0x017F)
```

**Note:** These values explain what the kernel outputs *during scanning*; they are not instructions to write a raw 16-bit value to GPIO. Use `fbctl` or `/dev/gpio/fb` for display experiments.

### Reading, writing, and control

Open the device with `open("/dev/gpio/fb", O_RDWR)`. Both **`read()` and `write()` require exactly 8 bytes**; other sizes return an error. Updating the data and enabling the display are separate operations.

| `ioctl()` command | Meaning |
|---|---|
| `OS_FB_ENABLE` | Enable scanning and reserve GP0–15 for the framebuffer |
| `OS_FB_DISABLE` | Blank the display, return GP0–15 to input mode, and release them |
| `OS_FB_STATUS` | Return 1 if enabled, otherwise 0 |

These constants are defined in `include/piconix.h`. Closing the descriptor **does not disable** the display; disable it explicitly when the GPIO pins are needed again. While enabled, normal GPIO operations on GP0–15 fail with **`OS_EBUSY`**, including operations on previously opened GPIO descriptors.

### fbctl command

```sh
fbctl status    # Show /proc/fb status and 8×8 pattern
fbctl on        # Enable the display
fbctl clear     # Zero the framebuffer without changing enable state
fbctl test      # Blink a heart three times; leave it displayed
fbctl off       # Blank the display and release GPIO pins
cat /proc/fb    # Read framebuffer state as text
```

`fbctl status` prints `/proc/fb`. Each row uses `#` for a lit LED and `.` for an unlit one. `fbctl clear` does not change the enabled state.

The `kitesys` demo displays digits, uses buttons on GP16/17 to shift the image, and drives a buzzer on GP18. It enables the framebuffer at startup; the display remains active after the program exits. Use `fbctl off` when finished.

**Electrical safety:** Each row anode may carry the current of multiple LEDs. Use current-limiting resistors and suitable driver circuitry rather than connecting the matrix directly to GPIO pins.

## GPIO

`kernel/pins.h` selects each pin's role using `GPn_MODE`. Run `ls /dev/gpio` to list GPIO pins. GPIO values, direction, and pull settings use the existing GPIO device interface. Pins assigned to ADC, PWM, or UART are hidden from the GPIO listing and rejected by GPIO operations.

## ADC and temperature

| Device | Input | Output |
|---|---|---|
| `/dev/adc/0` (disabled) | GP26 | Raw 12-bit ADC value (0–4095) |
| `/dev/adc/1` (disabled) | GP27 | Raw 12-bit ADC value (0–4095) |
| `/dev/adc/2` | GP28 | Raw 12-bit ADC value (0–4095) |
| `/dev/adc/temp` | On-chip sensor | Approximate Celsius temperature |

```sh
cat /dev/adc/2
cat /dev/adc/temp
watch 1 cat /dev/adc/temp
```

An ADC sample is captured at the first `read()` and returned as decimal text. Short reads drain the same sample; after its final byte, the next `read()` obtains a new sample. Temperature assumes a 3.3-V reference and is not a precision measurement. Reassigning GP26–28 to GPIO hides the corresponding ADC devices. The on-chip temperature sensor remains independent of pin assignments.

## PWM

By default, GP18 and GP19 are PWM outputs sharing one RP2040 PWM slice: **frequency is common**, while duty cycles are independent. On boot the frequency is **1 kHz** and both duties are **0%**.

PWM slice 1 channel A (`/dev/pwm/1/a`) maps to GPIO18 and channel B (`/dev/pwm/1/b`) to GPIO19. Both channels share one frequency.

```sh
cat /dev/pwm/1/freq
cat /dev/pwm/1/a
cat /dev/pwm/1/b
echo 2000 > /dev/pwm/1/freq
echo 50 > /dev/pwm/1/a
echo 0 > /dev/pwm/1/a
```

`/dev/pwm/1/freq` accepts 8–100000 Hz; `/dev/pwm/1/a` and `/dev/pwm/1/b` accept 0–100% duty as decimal text. Hardware dividers and counter limits may introduce frequency rounding. PWM device entries appear only for pins configured as PWM.

## I²C1

GP26 (SDA) and GP27 (SCL) are dedicated to I²C1. Access the bus through `/dev/i2c/1`, use `i2cctl scan` to discover devices, or run `bh1750 5` for an ambient-light sensor example. See the [I²C1 guide](I2C.md) for wiring, pull-ups, and C/Python access.

## Pin assignments

Edit `GPn_MODE` in `kernel/pins.h`, then rebuild. Available modes are `PIN_GPIO`, `PIN_ADC`, `PIN_PWM`, `PIN_UART`, and `PIN_I2C`; unsupported combinations are rejected at compile time. The same settings control listings, opening paths, and hardware initialization.

```c
#define GP18_MODE PIN_PWM
#define GP19_MODE PIN_PWM
#define GP26_MODE PIN_I2C
#define GP27_MODE PIN_I2C
#define GP28_MODE PIN_ADC
#define PWM_DEFAULT_FREQ 1000u
#define PWM_DEFAULT_DUTY 0u
```

GP26/27 are fixed to I²C1; changing them to `PIN_GPIO` or `PIN_ADC` is rejected at build time. ADC2 on GP28 remains available. The main console uses USB CDC; GP0/1 are GPIO pins for the LED matrix, and GP20/21 are reserved for UART1. UART-assigned pins are not exposed as GPIO devices.

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

## UART and console

The main console uses USB CDC only. UART0 console builds are not supported. UART1 is accessible as `/dev/ttyS1`; `getty /dev/ttyS1 sh &` starts another shell. See [UART.md](UART.md).

## Constraints

- GPIO cannot simultaneously own pins used by ADC, PWM, or I²C.
- GP18 and GP19 cannot use independent PWM frequencies.
- `/dev/mem` permits reading supported Flash/SRAM ranges. Writes are restricted to task slots (0x2000E000–0x20031FFF) shared memory (0x20032000–0x200327FF), and RAM-file payloads. Other kernel memory, trace, Wi-Fi reserve, and Flash are not writable. Writing a live task may crash the system.
- `/dev/trace` provides binary records; see [TRACE.md](TRACE.md).
- Drivers live in `kernel/gpio.c`, `kernel/adc.c`, and `kernel/pwm.c`; FD management stays in `kernel/kernel.c`.

## Interactive Framebuffer Editor (`fbed`)

`fbed` edits the 8×8, 64-bit (8-byte) framebuffer at `/dev/gpio/fb`. Each byte represents a row, with bit 7 at the left. It reads the current image and writes all eight bytes after each edit.

```text
piconix# fbed
```

| Key | Action |
|---|---|
| Up / Down / Left / Right | Move the selection |
| Space | Toggle the selected LED |
| `c` | Clear all pixels |
| `q` | Quit |

Each row's byte is displayed in hexadecimal. The editor enables framebuffer scanning when launched; the image and display remain active after exit. GP0–GP15 are reserved while framebuffer scanning is enabled. Run `fbctl off` to stop scanning and release those pins.

Both utilities require ANSI terminal support and character-at-a-time input. In VS Code Serial Monitor, disable the monitor's line-input mode.

## Interactive GPIO editor (`gpio`)

Run `gpio` in a USB CDC or UART1 ANSI terminal. It reports live pin state and refreshes about every 50 ms.

### Interactive `gpio` editor

GPIO pins are displayed as an eight-row, four-column grid (31–24, 23–16, 15–8, 7–0). Unavailable positions display `---` and cannot be selected. Arrow keys follow the screen layout. Keys: `i` input, `o` output, `h` HIGH, `l` LOW, `SP` toggle output, `u` pull-up, `d` pull-down, `n` no pull, `q` quit. HIGH/LOW/toggle apply to outputs; pull operations apply to inputs.

Labels such as `[FB]`, `[PWM]`, `[UART-TX]`, and `[UART-RX]` reflect live kernel-reported pin ownership. Such pins can be inspected but not edited. Releasing the framebuffer changes the displayed status automatically.

GPIO defaults at boot: GP25 is output (LOW), GP16 and GP17 are inputs with pull-ups. They remain editable with `gpio`.

For the kernel UDP 2000 remote-device interface, see [Remote device control](REMOTE-DEVICE.md).
