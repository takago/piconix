# 3. Devices and I/O

[Learning path](LEARNING-PATH.md) | [日本語](../../ja/literacy/DEVICE-IO.md) | [Device reference](../reference/devices/DEVICES.md)

## Goal and prerequisites

**Goal:** Understand why peripherals are exposed through `/dev` with file-like I/O operations. The first exercise only requires USB. GPIO, I2C, UART, and LED matrix experiments need verified wiring: read the [hardware guide](../reference/hardware/HARDWARE.md) first.

## Lab A: Find the devices (no external circuit)

```text
piconix# ls /dev
piconix# ls /dev/gpio
piconix# cat /proc/fb
piconix# tty
```

`/proc/fb` exposes the LED framebuffer as text. **A valid framebuffer does not prove the physical LED wiring is correct.** Distinguish terminal output from controlling a physical device.

## Lab B: Control connected hardware (optional)

If an LED matrix is correctly connected, consult the [framebuffer instructions](../reference/devices/DEVICES.md) before using `fbctl`. For GPIO, I2C, and UART follow the [device guide](../reference/devices/DEVICES.md), [I2C guide](../reference/devices/I2C.md), and [UART guide](../reference/devices/UART.md). **Do not drive unverified wiring or apply voltages above the RP2040's 3.3 V logic level.**

## Observe and explain

- How do the effects of reading/writing `/tmp` differ from `/dev`?
- When might `ioctl()` be needed in addition to `read()` and `write()`?
- What can software observe even when hardware is not connected?

## Next step

Use the [C application guide](../development/apps/c/PROGRAMMING.md) to access devices, then explore [adding a driver](../development/drivers/DRIVER-DEVELOPMENT.md).
