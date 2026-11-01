# Pico W network access

[日本語](../ja/NETWORK.md) | [Setup](SETUP.md)

The Pico W build defaults to Wi-Fi AP mode with a small DHCP server; it can also join an existing network in build-time STA mode. The network shell is independent of the USB CDC console. The ordinary Pico has no Wi-Fi support. The current Pico W firmware has also been tested successfully on a regular Pico, but Wi-Fi is unavailable on that hardware.

## Build

With Pico SDK 2.1.1 installed:

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

Set `PICO_TOOLCHAIN_PATH` if using a separately installed toolchain. The UF2 is `build-pico_w/piconix.uf2`. Flash it in BOOTSEL mode. `PICONIX_WIFI_LAYOUT` is obsolete; use `-DPICO_BOARD=pico_w`. For the regular Pico use `-DPICO_BOARD=pico`.

For STA build options and connection status, see [Wi-Fi modes](WIFI-MODES.md).

## Wi-Fi settings

- SSID: `PicoNIX-XXXXXXXX` (device-dependent suffix; check startup output)
- Password: `piconix1234`
- AP IP: `192.168.4.1`
- DHCP client IP: `192.168.4.2` (single-client configuration)

Connect to the AP; the IP address is normally assigned automatically. This AP does not provide Internet access.

## Remote terminal

| Protocol | Port | Command |
|---|---:|---|
| Telnet | TCP 23 | `telnet 192.168.4.1` |

The network shell uses the `/dev/ttyN0` TTY and permits one active network terminal session at a time. USB CDC remains independent.

### Disconnecting

**Telnet:** press `Ctrl + ]`, then type `quit` and Enter at the `telnet>` prompt.

**socat:** when started with `escape=0x1d`, then press `Ctrl + ]`. In raw mode, `Ctrl + C` is normally sent as data rather than terminating socat.

On Linux, install clients as needed (e.g. `sudo apt install telnet socat`). On Windows 10/11, enable the optional Telnet Client feature or run `dism /online /Enable-Feature /FeatureName:TelnetClient` in an elevated shell.

## Notes and security

Telnet and raw TCP are unencrypted. Use only on a trusted experimental network. The AP password is a published fixed value; do not send secrets. Terminal behavior can vary with the Telnet client. For maintenance notes about DHCP failures, see the [historical DHCP investigation](../history/ja/NETWORK-DHCP-NOTES.md).

## TCP/UDP applications and remote device control

Alongside Telnet (TCP 23), Pico W supports user-space `nc`, `httpd` (TCP 80), `tcp_echod`, and `udp_echod`. UDP 2000 offers remote access to permitted device paths. See [Remote device control](REMOTE-DEVICE.md) for the UDP protocol and [Socket API and network applications](NETWORK-PROGRAMMING.md) for user-space sockets.

Wi-Fi STA reassociation can be requested with `echo restart > /dev/net/wifi`. The request is processed in the network poll loop; an active remote session may disconnect. `/dev/ttyN0` remains the terminal device.

## Polling and Wi-Fi stability

PicoNIX uses the polling-based CYW43/lwIP integration. `net_poll()` runs in the idle task, **not** in the 1 ms timer interrupt. It calls `cyw43_arch_poll()` and runs the network services only when at least **250 µs** have elapsed since the previous network poll. This interval applies to both STA and AP modes; it is a **minimum interval**, not a guaranteed 250 µs schedule. Other tasks can delay the idle task.

The setting is defined by `NET_POLL_INTERVAL_US` in `kernel/net.c`. Its history and rationale are:

- Initially, polling was unrestricted, with roughly 100,000 calls/s observed during idle time.
- A 1 ms minimum interval reduced calls to at most approximately 1,000/s, but interactive Telnet responsiveness appeared worse.
- 500 µs (at most approximately 2,000 calls/s) improved perceived responsiveness.
- 250 µs (at most approximately 4,000 calls/s) is the current setting, chosen for responsiveness; its performance and long-term stability have not been systematically measured.

For STA, PicoNIX explicitly disables CYW43 Wi-Fi power saving using `CYW43_NONE_PM`. The idle task stack is 4 KiB. `/proc/net/wifi` reports `POLL_COUNT` (cumulative **executed** polls), `POLL_MAX_GAP_MS` (largest observed gap), `PM_ERROR` (STA power-management setup result), and `IDLE_STACK` (estimated stack usage). `POLL_COUNT` does not reset when the STA connection is restarted and does **not** count received packets.

To request a manual STA reconnect, use `echo restart > /dev/net/wifi`. This does not restart the OS or reset the network poll counter; an existing Telnet session may disconnect.
