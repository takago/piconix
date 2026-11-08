# Pin Assignments and Board Header

*Technical reference*

**`kernel/pins.h` is the source of truth** for configured pin modes. This page has been checked against the supplied `GPn_MODE` definitions. The `pins` command also displays configured assignments. Consult Pico/Pico W board documentation for physical pin numbering.

## Default pin roles

| GPIO | Configured mode | Role and caveats |
|---|---|---|
| GP0–15 | `PIN_GPIO` | Reserved for the LED matrix while its framebuffer is enabled |
| GP16, GP17 | `PIN_GPIO` | Pushbutton example; normally input with pull-up |
| GP18, GP19 | `PIN_PWM` | PWM slice 1 A/B; share a frequency |
| GP20, GP21 | `PIN_UART` | UART1 TX/RX; no UART0 console |
| GP22 | `PIN_GPIO` | General-purpose GPIO |
| GP23–25 | `PIN_GPIO` in `pins.h` | On Pico W, reserved for CYW43 and blocked from generic GPIO device access |
| GP26, GP27 | `PIN_I2C` | I²C1 SDA/SCL; ADC0/1 are not enabled |
| GP28 | `PIN_ADC` | ADC2 external input |

**Pico W caution:** Even when GP23–25 have `PIN_GPIO` in `pins.h`, the Pico W build reserves them for CYW43. The `/dev/gpio/23`–`25` nodes are not exposed and `pins` reports reserved status. GP29 is also used internally. The Pico W onboard LED uses CYW43 `WL_GPIO0`, not RP2040 GP25.

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

On Pico W, GP23–25 are controlled by the wireless interface and protected from generic GPIO access. GP25 directly controls the onboard LED only on the original Pico. The Pico W LED is connected through CYW43 `WL_GPIO0`.

## Changing pin modes

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

## Connection examples

See [WIRING.md](WIRING.md) for physical LED module connections, pushbuttons, PWM, UART, I²C, and ADC.

---

**Related links:** [Hardware index](README.md) | [Device usage](../devices/README.md) | [日本語](../../../ja/reference/hardware/PINS.md)
