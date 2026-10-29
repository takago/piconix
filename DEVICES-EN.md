# PicoNIX Device Guide

[日本語](DEVICES-JP.md) | [README](README-EN.md)

PicoNIX exposes hardware through virtual files under `/dev`. These are not stored on disk: the kernel routes `open()`, `read()`, and `write()` to the device implementations.

## Device overview

| Path | Purpose |
|---|---|
| `/dev/console` | Main console (USB CDC or UART0 build) |
| `/dev/uart1` | UART1 terminal |
| `/dev/gpio/fb` | 64-bit framebuffer for an 8×8 LED matrix |
| `/dev/gpio/` | Pins configured for GPIO in `pins.h` |
| `/dev/adc/` | Enabled ADC inputs and on-chip temperature sensor |
| `/dev/pwm/` | Enabled PWM outputs and common frequency |
| `/dev/mem` | Read-only access to permitted RAM/flash ranges |
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

`src/pins.h` selects each pin's role using `GPn_MODE`. Run `ls /dev/gpio` to list GPIO pins. GPIO values, direction, and pull settings use the existing GPIO device interface. Pins assigned to ADC, PWM, or UART are hidden from the GPIO listing and rejected by GPIO operations.

## ADC and temperature

| Device | Input | Output |
|---|---|---|
| `/dev/adc/0` | GP26 | Raw 12-bit ADC value (0–4095) |
| `/dev/adc/1` | GP27 | Raw 12-bit ADC value (0–4095) |
| `/dev/adc/2` | GP28 | Raw 12-bit ADC value (0–4095) |
| `/dev/adc/temp` | On-chip sensor | Approximate Celsius temperature |

```sh
cat /dev/adc/0
cat /dev/adc/temp
watch 1 cat /dev/adc/temp
```

An ADC sample is captured when the file is opened and returned as decimal text. Temperature assumes a 3.3-V reference and is not a precision measurement. Reassigning GP26–28 to GPIO hides the corresponding ADC devices. The on-chip temperature sensor remains independent of pin assignments.

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

## Pin assignments

Edit `GPn_MODE` in `src/pins.h`, then rebuild. Available modes are `PIN_GPIO`, `PIN_ADC`, `PIN_PWM`, and `PIN_UART`; unsupported combinations are rejected at compile time. The same settings control listings, opening paths, and hardware initialization.

```c
#define GP18_MODE PIN_PWM
#define GP19_MODE PIN_PWM
#define GP26_MODE PIN_ADC
#define GP27_MODE PIN_ADC
#define GP28_MODE PIN_ADC
#define PWM_DEFAULT_FREQ 1000u
#define PWM_DEFAULT_DUTY 0u
```

To restore all general-purpose pins to GPIO mode, set the ADC/PWM assignments to `PIN_GPIO`. With the default USB CDC console, GP0/1 are available as GPIO and GP20/21 are reserved for UART1. For a UART0 console build, change GP0/1 to `PIN_UART`. UART-assigned pins are not exposed as GPIO devices.

## Default wiring overview

| Pins | Function | Notes |
|---|---|---|
| GP0–7 | 8×8 LED matrix columns (cathodes) | Active LOW |
| GP8–15 | 8×8 LED matrix rows (anodes) | Active HIGH |
| GP16/17 | Push buttons | To GND, internal pull-up |
| GP18/19 | PWM | Shared frequency |
| GP20/21 | UART1 TX/RX | 115200 baud, 8N1 |
| GP26–28 | ADC0–2 | 0–3.3 V input |

Use a common GND and 3.3 V logic for UART; do not connect RS-232 voltages directly. An LED matrix requires current-limiting resistors and suitable driver circuitry. Use `pins` to inspect the pin assignment.

## UART and console

The default console uses USB CDC. UART0 console builds require GP0/1 to be `PIN_UART` in `pins.h`. UART1 is accessible as `/dev/uart1`; `getty /dev/uart1 sh &` starts another shell. See [UART-EN.md](UART-EN.md).

## Constraints

- GPIO cannot simultaneously own pins used by ADC or PWM.
- GP18 and GP19 cannot use independent PWM frequencies.
- `/dev/mem` is read-only and limited to permitted memory ranges.
- `/dev/trace` provides binary records; see [TRACE-EN.md](TRACE-EN.md).
- Drivers live in `src/gpio.c`, `src/adc.c`, and `src/pwm.c`; FD management stays in `src/kernel.c`.
