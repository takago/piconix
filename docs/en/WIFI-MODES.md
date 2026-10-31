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

Fields are `MODE`, `SSID`, `MAC`, `IP`, `NETMASK`, `GATEWAY`, and `RSSI`. RSSI is queried on read in STA mode; AP mode displays `N/A`. On non-Wi-Fi Pico builds, the file reports `MODE: N/A`.

The RAM filesystem also contains `/tmp/test.s` and `/tmp/test.py` at boot. Both display a face three times with one-second pauses, then print `...bye`. Assemble the first with `as test.s` and run the output with `./a.out`; run the second with `python test.py`.
