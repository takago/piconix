# Hardware Overview

*Technical reference*

## Supported boards

PicoNIX targets the RP2040-based **Raspberry Pi Pico and Pico W**. The board is selected at build time. Starting the system and using the USB CDC console requires no external LED matrix or sensor.

- Pico: USB CDC and local interfaces such as UART1.
- Pico W: adds wireless networking. Header positions are shared, but onboard pin usage differs.

## Hardware versus OS interfaces

| Question | Reference |
|---|---|
| How is each GPn assigned? | [Pin assignments](PINS.md); source of truth: `kernel/pins.h` |
| How should components be connected? | [Wiring](WIRING.md), plus each component's datasheet |
| How do `open()`, `read()`, and `write()` access `/dev`? | [Device reference](../devices/README.md) |
| How are device drivers implemented or changed? | [Driver development](../../development/drivers/DRIVER-DEVELOPMENT.md) |

Physical GPIO resources cannot be used by multiple features simultaneously. When the LED matrix is active it reserves GP0–15; run `fbctl off` to release them.

## Before connecting hardware

1. Unplug the USB power source before changing wires.
2. Check [PINS.md](PINS.md), distinguishing GPn numbers from **physical header pin numbers**.
3. Review [WIRING.md](WIRING.md) for 3.3 V logic, common ground, current limiting, and pull-ups.
4. Confirm polarity, physical pinout, and ratings against the exact parts and their datasheets before applying power.
5. Boot the minimal board first, then attach optional peripherals one by one.

**Caution:** The LED matrix pin table illustrates one specific part; it does not constitute a complete safe driver circuit.

---

**Related links:** [Hardware index](README.md) | [日本語](../../../ja/reference/hardware/HARDWARE.md) | [Documentation index](../../README.md) | [Setup](../../setup/SETUP.md)
