# Hardware — Technical Reference

Use this index for **boards and physical connections**. The [device reference](../devices/README.md) explains `/dev` operations; the [driver development guide](../../development/drivers/DRIVER-DEVELOPMENT.md) explains implementing them in the kernel.

| Topic | Document |
|---|---|
| Board support, hardware assumptions, and getting started safely | [Hardware overview](HARDWARE.md) |
| 40-pin layout, GPn assignments and reserved pins | [Pin assignments and board header](PINS.md) |
| LED matrix, switches, PWM, UART and I²C connections | [Wiring and electrical precautions](WIRING.md) |

**References:** [Overview](HARDWARE.md) → [Pin assignments](PINS.md) → [Wiring](WIRING.md) → [Device usage](../devices/README.md).

Cross-check `kernel/pins.h` (pin modes), `kernel/fb.c` (LED scanning), and the actual component datasheets.

---

**Related links:** [Reference index](../README.md) | [Devices and I/O](../devices/README.md) | [日本語](../../../ja/reference/hardware/README.md)
