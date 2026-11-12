# Devices and I/O — Technical Reference

This is the **canonical inventory and detailed reference** for PicoNIX device nodes. The nodes under `/dev` are interfaces to kernel drivers, not ordinary RAM files.

**Related topics:** [CDC virtual console](CDC-VCONSOLE.md) | [PTY](PTY.md) | [I²C1](I2C.md) | [UART1](UART.md) | [Hardware and wiring](../hardware/README.md) | [Filesystem](../FILESYSTEM.md) | [Networking](../network/README.md)

For the full directory and node inventory, access modes, and RAM file limits, see [Filesystem and device nodes](../FILESYSTEM.md).

PicoNIX exposes hardware through virtual files under `/dev`. These are not stored on disk: the kernel routes `open()`, `read()`, and `write()` to the device implementations.

## Complete device-node inventory

This table lists **device types and path patterns in the standard configuration**. `N` is a GPIO number. Peripheral assignments determine whether a given node exists. Use `ls /dev` and `ls` on its subdirectories to inspect the current build. `/proc` entries are status pseudo-files, not `/dev` devices (see [all VFS nodes](../FILESYSTEM.md)).

| Node | Read | Write/control | Data and purpose | Details |
|---|:---:|:---:|---|---|
| `/dev/console` | Yes | Yes | USB CDC text terminal | [TTY](../commands/TTY.md) |
| `/dev/ttyV1` | Yes | Yes | Second CDC virtual console (Ctrl+T) | [CDC virtual console](CDC-VCONSOLE.md) |
| `/dev/ttyS1` | Yes | Yes | UART1 text terminal | [UART](UART.md) |
| `/dev/ptmx` | Yes | Yes | PTY master (one exclusive pair) | [PTY](PTY.md) |
| `/dev/pts/0` | Yes | Yes | PTY slave (available while master is open) | [PTY](PTY.md) |
| `/dev/pts/0.screen` | Yes | — | Read-only PTY 80x24 text screen | [PTY](PTY.md) |
| `/dev/pts/0.key` | — | Yes | Write named PTY keys | [PTY](PTY.md) |
| `/dev/ttyN0` | Yes | Yes | Network terminal (Pico W) | [Networking](../network/NETWORK.md) |
| `/dev/net/wifi` | — | Control | STA reconnect request (Pico W, STA mode) | [Wi-Fi](../network/WIFI-MODES.md) |
| `/dev/net/packet` | Yes | — | IPv4 packet information (Pico W) | [tcpdump](../network/TCPDUMP.md) |
| `/dev/net/icmp` | Yes | Yes | ICMP Echo packets (Pico W, exclusive access) | [ping](../network/PING.md) |
| `/dev/rand` | Yes | — | Non-cryptographic pseudorandom bytes, up to 4 per read | [Filesystem](../FILESYSTEM.md) |
| `/dev/fb` | Yes | Yes | Eight binary LED-frame bytes; enable via `ioctl` | [Framebuffer below](#8×8-led-framebuffer) |
| `/dev/fbchar` | Yes | Yes | ASCII glyph or named pattern | [Text/pattern device below](#text-and-symbol-display-devfbchar) |
| `/dev/gpio/N` | Yes | Yes | GPIO logic value (configured pins only) | [Pin assignment](../hardware/PINS.md) |
| `/dev/gpio/N.dir` | Yes | Yes | GPIO direction | [Pin assignment](../hardware/PINS.md) |
| `/dev/gpio/N.pull` | Yes | Yes | GPIO pull-up/pull-down | [Pin assignment](../hardware/PINS.md) |
| `/dev/adc/0`, `1`, `2` | Yes | — | ADC decimal integer (enabled inputs only) | [ADC below](#adc-and-temperature-sensor) |
| `/dev/adc/temp` | Yes | — | On-chip approximate temperature | [ADC below](#adc-and-temperature-sensor) |
| `/dev/pwm/1/freq` | Yes | Yes | Shared PWM frequency (Hz) | [PWM below](#pwm) |
| `/dev/pwm/1/a`, `1/b` | Yes | Yes | PWM duty cycle (%) | [PWM below](#pwm) |
| `/dev/i2c/1` | Yes | `ioctl` | I²C1 master, GP26/27 | [I²C](I2C.md) |
| `/dev/mem` | Yes | Restricted | Addressed Flash/SRAM access | [Memory layout](../MEMORY-LAYOUT.md) |
| `/dev/shm` | Yes | Yes | Shared 2-KiB RAM; `lseek` supported | [Memory layout](../MEMORY-LAYOUT.md) |
| `/dev/trace` | Yes | — | Binary trace events | [trace](../commands/TRACE.md) |

`—` means unsupported. `/dev/net/*` and `/dev/ttyN0` depend on Pico W support. GPIO, ADC and PWM paths depend on `kernel/pins.h`. **Do not confuse text devices suitable for `cat`/`echo` with fixed-length binary interfaces or `ioctl()`-based devices.**

## 8×8 LED framebuffer

PicoNIX provides a monochrome 8×8 LED-matrix framebuffer connected to GP0–15. Its device path is **`/dev/fb`**, not `/dev/gpio/fb` or `/dev/fb0`. The default wiring assumes the main console uses USB CDC.

### Data layout

The framebuffer holds **64 bits (8 bytes)**: one byte per row. `buffer[0]` through `buffer[7]` correspond to GP8 through GP15 (row anodes). Bits 0–7 in each byte correspond to GP0–GP7 (column cathodes). A set bit turns its LED **on**. In the text view provided by `/proc/fb`, bit 7 appears at the left and bit 0 at the right.

A SysTick handler scans **one row every 1 ms**, completing a frame in approximately 8 ms (**125 Hz**). There is no brightness control. Anodes are blanked before each row change to reduce ghosting. See [LED wiring, polarity, and coordinates](../hardware/WIRING.md) and [pin assignments](../hardware/PINS.md) for physical details.

### First display: a quick check

Check [LED matrix wiring and electrical safety](../hardware/WIRING.md) before running these commands.

```sh
fbctl test
cat /proc/fb
fbctl off
```

The first command displays a heart. The second prints the eight stored pixel rows; the third stops scanning and releases GP0–15 for GPIO use. **`cat /proc/fb` shows stored bytes, not proof that the physical LEDs are wired correctly.** Check the actual display visually.

### Reading, writing, and control

Open the device with `open("/dev/fb", O_RDWR)`. Both **`read()` and `write()` require exactly 8 bytes**; other sizes return an error. Updating the data and enabling the display are separate operations.

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

**Electrical safety:** The matrix requires current limiting and suitable driver circuitry. See [wiring and electrical safety](../hardware/WIRING.md).

### Interactive Framebuffer Editor (`fbed`)

`fbed` edits the 8×8, 64-bit (8-byte) framebuffer at `/dev/fb`. Each byte represents a row, with bit 7 at the left. It reads the current image and writes all eight bytes after each edit.

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

### Text and symbol display (`/dev/fbchar`)

`/dev/fbchar` controls the same 8×8 LED as `/dev/fb`, but accepts text rather than eight binary framebuffer bytes. A successful write automatically enables LED scanning, and the display persists after `close()`.

```sh
cat /dev/fbchar
echo A > /dev/fbchar
echo heart > /dev/fbchar
echo clear > /dev/fbchar
fbctl off
```

Writes accept **one printable ASCII character** (32–126) or one of `heart`, `star`, `smile`, `sad`, `up`, `down`, `left`, `right`, `check`, `cross`, `clear`. `clear` replaces the image with blank pixels but does not turn scanning off. Unknown names and oversized writes fail without updating the display. Trailing CR/LF is ignored. Reading returns a finite usage/pattern list, then EOF. See `/man/led-patterns` on PicoNIX for the ROM reference.

## GPIO

`kernel/pins.h` selects each pin's role using `GPn_MODE`. See [pin assignments](../hardware/PINS.md) for board-level details. Run `ls /dev/gpio` to list GPIO pins. GPIO values, direction, and pull settings use the existing GPIO device interface. Pins assigned to ADC, PWM, or UART are hidden from the GPIO listing and rejected by GPIO operations.

### Interactive GPIO editor (`gpio`)

Run `gpio` in a USB CDC or UART1 ANSI terminal. It reports live pin state and refreshes about every 50 ms.

#### Interactive `gpio` editor

GPIO pins are displayed as an eight-row, four-column grid (31–24, 23–16, 15–8, 7–0). Unavailable positions display `---` and cannot be selected. Arrow keys follow the screen layout. Keys: `i` input, `o` output, `h` HIGH, `l` LOW, `SP` toggle output, `u` pull-up, `d` pull-down, `n` no pull, `q` quit. HIGH/LOW/toggle apply to outputs; pull operations apply to inputs.

Labels such as `[FB]`, `[PWM]`, `[UART-TX]`, and `[UART-RX]` reflect live kernel-reported pin ownership. Such pins can be inspected but not edited. Releasing the framebuffer changes the displayed status automatically.

GPIO defaults at boot: GP25 is output (LOW), GP16 and GP17 are inputs with pull-ups. They remain editable with `gpio`.

**Terminal note:** Both `fbed` and `gpio` need ANSI terminal support and character-at-a-time input. In VS Code Serial Monitor, disable line-input mode.

For the kernel UDP 2000 remote-device interface, see [Remote device control](../network/REMOTE-DEVICE.md).

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

PWM slice 1 channel A (`/dev/pwm/1/a`) maps to GPIO18 and channel B (`/dev/pwm/1/b`) to GPIO19.

```sh
cat /dev/pwm/1/freq
cat /dev/pwm/1/a
cat /dev/pwm/1/b
echo 2000 > /dev/pwm/1/freq
echo 50 > /dev/pwm/1/a
echo 0 > /dev/pwm/1/a
```

`/dev/pwm/1/freq` accepts integer frequencies from 8 to 100000 Hz. `/dev/pwm/1/a` and `/dev/pwm/1/b` accept duty cycles from 0 to 100% with one decimal place (for example `7.5`), and display one decimal place when read. Hardware divider and counter limits can cause frequency rounding. PWM device entries appear only for pins configured as PWM.

## I²C1

GP26 (SDA) and GP27 (SCL) are dedicated to I²C1. Access the bus through `/dev/i2c/1`, use `i2cctl scan` to discover devices, or run `bh1750 5` for an ambient-light sensor example. See [hardware wiring](../hardware/WIRING.md) for connections and pull-ups, and the [I²C1 guide](I2C.md) for C/Python access.

## UART and console

The main console uses USB CDC only. UART0 console builds are not supported. UART1 is accessible as `/dev/ttyS1`; `getty /dev/ttyS1 sh &` starts another shell. See [UART.md](UART.md) and [UART wiring](../hardware/WIRING.md).

## Constraints

- GPIO cannot simultaneously own pins used by ADC, PWM, or I²C.
- GP18 and GP19 cannot use independent PWM frequencies.
- `/dev/mem` permits reading supported Flash/SRAM ranges. Writes are restricted to task slots (0x2000E000–0x20031FFF) shared memory (0x20032000–0x200327FF), and RAM-file payloads. Other kernel memory, trace, Wi-Fi reserve, and Flash are not writable. Writing a live task may crash the system.
- `/dev/trace` provides binary records; see [TRACE.md](../commands/TRACE.md).
- Drivers live in `kernel/gpio.c`, `kernel/adc.c`, and `kernel/pwm.c`; FD management stays in `kernel/kernel.c`.

---

**Related links:** [Reference index](../README.md) | [日本語](../../../ja/reference/devices/README.md) | [Hardware and wiring](../hardware/README.md)

---

**Related links:** [Reference index](../README.md) | [日本語](../../../ja/reference/devices/README.md)
