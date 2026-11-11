# Networking — Technical Reference

This directory documents Pico W Wi-Fi, TCP/UDP networking and network services. The standard Pico does not provide the Pico W Wi-Fi and network services.

## Network diagnostic commands

These commands observe **different network layers**:

1. [`ip`: address and network interfaces](IP.md)
2. [`ping`: ICMP reachability](PING.md)
3. [`tcpdump`: TCP/UDP/ICMP packet summaries](TCPDUMP.md)
4. [`net`: Wi-Fi, AP stations and ARP](NET.md)

Start with [Network reference](NETWORK.md). These diagnostics require a **Pico W**, not the regular Pico.


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

**Related references:** [Network reference](NETWORK.md) → [Network development](../../development/network/NETWORK-DEVELOPMENT.md).

---

**Related links:** [Reference index](../README.md) | [Devices](../devices/README.md) | [日本語](../../../ja/reference/network/README.md)
