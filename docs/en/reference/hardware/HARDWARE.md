# Hardware and Wiring

[Documentation index](../../../DOCS.md) | [Device interface](../devices/DEVICES.md) | [日本語](../../../ja/reference/hardware/HARDWARE.md)

This page is the entry point for the board and circuit setup expected by PicoNIX. Verify pin assignments against the **current source**, especially `kernel/pins.h` and the board selection in `CMakeLists.txt`, before connecting hardware.

- [Device overview, pin roles, and wiring](../devices/DEVICES.md)
- [I²C connections](../devices/I2C.md)
- [UART wiring](../devices/UART.md)
- [Build for Pico or Pico W](../../setup/SETUP.md)

Hardware interface use belongs in `reference/devices/`; instructions for adding kernel-side drivers belong in [driver development](../../development/drivers/DRIVER-DEVELOPMENT.md).
