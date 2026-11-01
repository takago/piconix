# Wi-Fi modes and status

Pico W builds default to **AP mode**. PicoNIX creates its own `PicoNIX-XXXXXXXX` Wi-Fi network and provides DHCP to one client. The existing Telnet (TCP 23), remote device control (UDP 2000), and user-process socket applications continue to use the same network stack.

To join an existing Wi-Fi network instead, configure a Pico W build with:

```sh
cmake -S . -B build-sta -DPICO_BOARD=pico_w \
  -DPICONIX_WIFI_MODE=STA \
  -DPICONIX_WIFI_SSID='YourNetwork' \
  -DPICONIX_WIFI_PASSWORD='YourPassword'
```

STA mode uses the Pico SDK's Wi-Fi connection helper and DHCP client. An unsuccessful connection reports an error rather than silently switching to AP mode. Use a separate build directory for each mode.

**Security:** The STA password is embedded in the firmware and stored in the CMake cache/build metadata. Keep the build directory private, do not commit it, and do not distribute UF2 files containing private credentials. Prefer a local, trusted network; PicoNIX's Telnet and UDP device-control services do not provide strong authentication or encryption.

Read the current interface state using:

```sh
cat /proc/net/wifi
```

The status includes `MODE`, `STATUS`, `SSID`, `ATTEMPTS`, `CONNECT_ERROR`, `WIFI_LINK`, `TCPIP_LINK`, `DHCP_BOUND`, `IP`, `NETMASK`, and `GATEWAY` (some fields depend on mode and connection state). In STA mode, successful connection detection uses the TCP/IP link and DHCP lease state. On non-Wi-Fi Pico builds, the file reports `MODE: N/A`.
