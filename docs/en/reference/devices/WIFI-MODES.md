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

## STA communication diagnostics

STA mode explicitly selects `CYW43_NONE_PM` after enabling the STA interface.
This disables Wi-Fi power saving for a reproducible comparison with AP mode.
AP mode is unchanged. The idle/network stack is 4 KiB; its use is sampled
with a high-water mark when reading `/proc/net/wifi`.

The additional STA-only fields are:

- `PM_ERROR`: `0` means the SDK accepted the no-power-save setting.
- `POLL_COUNT`: how often the idle task entered `net_poll()` since startup.
- `POLL_MAX_GAP_MS`: largest elapsed time between calls to `net_poll()`;
  the counter is not a packet reception/forwarding measurement.
- `IDLE_STACK`: approximate peak stack bytes used / 4096 bytes reserved.
  A value close to 4096 indicates a potential overflow. This watermark is
  diagnostic only; it cannot rule out overflows or measure the boot stack.

An `UP` status and a DHCP lease do **not** prove that ARP, ICMP, or TCP
packets can be exchanged. Test `ping` (on the Linux/host computer, **not** a PicoNIX command) and Telnet from the same network,
repeat after several power cycles, and compare with the previous firmware.
For stalled communication, capture ARP/ICMP traffic on the host.

The firmware must still pass the linked ELF's 56-KiB kernel SRAM limit checks.
Build and hardware validation are required before treating the workaround
as a confirmed solution.


## MAC address and signal-strength indicator

`net` and `cat /proc/net/wifi` show the interface's own `MAC` address on
Pico W (in both AP and STA modes). The address is read from CYW43 by the
network polling task and cached; before the first successful read, `N/A`
is shown. It is **not** the router's MAC address.

In STA mode, `RSSI` is sampled approximately every 5 seconds while the
connection is up, for example:

```text
MAC: 28:CD:C1:12:34:56
RSSI: -44 dBm (100%)
```

The percentage is an **estimate derived from RSSI**, not a measurement
of Wi-Fi link quality or throughput. The integer conversion uses
`-100 dBm = 0%` and `-50 dBm or stronger = 100%`, with linear values
between these limits and clamping to the range 0–100%.
For example, `-67 dBm` corresponds to `66%`. Signal strength alone
cannot indicate interference, retransmissions, or packet loss.

`RSSI: N/A` means no valid measurement is available (not connected,
measurement failure, or before the first sample). AP mode has no upstream
access-point RSSI, so it shows `RSSI: N/A (AP mode)`.
The CYW43 query runs only in the idle network polling context,
not in `/proc` reads.

## mDNS hostnames

Pico W builds use the lwIP mDNS responder. In STA mode, after DHCP has assigned an address, the device advertises `piconix-xxxxxxxx.local`, where `xxxxxxxx` is the eight-digit lowercase hexadecimal Flash ID (for example, `piconix-4353ba2e.local`). In AP mode, the fixed hostname is `piconix.local`, and the AP address is `192.168.4.1`.

```sh
# STA mode: replace the example Flash ID with the device's ID
avahi-resolve-host-name piconix-4353ba2e.local
telnet piconix-4353ba2e.local

# From a client joined to PicoNIX AP
telnet piconix.local
```

`MDNS_NAME` in `/proc/net/wifi` is the advertised hostname; `MDNS_ACTIVE` reports responder registration and `MDNS_ERROR` reports the registration error. mDNS resolves names to IP addresses; it does not replace ARP resolution from IPv4 addresses to MAC addresses. Client-side mDNS support is required for `.local` names.

## STA gratuitous ARP (experimental)

After obtaining an IPv4 address through DHCP, STA mode sends a gratuitous ARP (GARP) every 60 seconds as an experimental workaround. `GARP_INTERVAL_S` reports the interval; `GARP_OK` and `GARP_ERROR` count the lwIP transmission API's results. API success does not prove delivery over the air or arrival at another host. AP mode does not perform these periodic transmissions.

In diagnostic testing, GARP packets arrived irregularly over one Linux Wi-Fi/AP combination, but regularly on a wired interface on that AP and over Wi-Fi with a different AP. This does not establish a PicoNIX timer defect; investigate access-point and client-driver combinations before drawing conclusions. Periodic GARP is not a permanent fix for ARP connectivity faults.


The additional `cat /proc/net/memory` snapshot is available in both AP and STA modes.
