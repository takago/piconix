# Networking — Technical Reference

**Document type: REFERENCE INDEX** | [Reference index](../README.md) | [Devices](../devices/README.md) | [Textbook](../../learn/README.md) | [日本語](../../../ja/reference/network/README.md)

This directory documents Pico W Wi-Fi, TCP/UDP networking and network services. Networking chapters are optional when using a standard Pico without Wi-Fi.

## Connection and diagnostics

- [Pico W network access](NETWORK.md)
- [Wi-Fi modes and status](WIFI-MODES.md)
- [`net` network diagnostics](NET.md)

## Applications and services

- [`nc` (netcat) and termination](NC.md)
- [User-space HTTP server (`httpd`)](HTTPD.md)
- [Remote device control via UDP port 2000](REMOTE-DEVICE.md)
- [Remote GPIO control over UDP](REMOTE_GPIO.md)

**Development guides:** [User-space C networking](../../development/apps/c/NETWORK-PROGRAMMING.md) | [Kernel network implementation and extension](../../development/network/NETWORK-DEVELOPMENT.md).

**Guided learning:** [B6 Networking](../../learn/B/B6.md) → [C5 Network implementation](../../learn/C/C5.md).
