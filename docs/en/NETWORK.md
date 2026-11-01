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

The network shell uses the `/dev/wifi` TTY and permits one active network terminal session at a time. USB CDC remains independent.

### Disconnecting

**Telnet:** press `Ctrl + ]`, then type `quit` and Enter at the `telnet>` prompt.

**socat:** when started with `escape=0x1d`, then press `Ctrl + ]`. In raw mode, `Ctrl + C` is normally sent as data rather than terminating socat.

On Linux, install clients as needed (e.g. `sudo apt install telnet socat`). On Windows 10/11, enable the optional Telnet Client feature or run `dism /online /Enable-Feature /FeatureName:TelnetClient` in an elevated shell.

## Notes and security

Telnet and raw TCP are unencrypted. Use only on a trusted experimental network. The AP password is a published fixed value; do not send secrets. Terminal behavior can vary with the Telnet client. For maintenance notes about DHCP failures, see the [historical DHCP investigation](../history/ja/NETWORK-DHCP-NOTES.md).

## TCP/UDP applications and remote device control

Alongside Telnet (TCP 23), Pico W supports user-space `nc`, `httpd` (TCP 80), `tcp_echod`, and `udp_echod`. UDP 2000 offers remote access to permitted device paths. See [Socket API and network applications](NETWORK-PROGRAMMING.md).
