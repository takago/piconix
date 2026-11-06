# `ip`: inspect IPv4 addresses and interfaces

PicoNIX's `ip` is a **read-only summary**, not Linux `iproute2`. It cannot configure addresses or routes. Use it to identify the Pico W address before network tests.

## Commands on the PicoNIX shell

```text
piconix# ip
piconix# ip addr
piconix# ip -h
```

The first two forms give the same summary, parsed from `/proc/net/wifi`. On a regular Pico, the Wi-Fi interface is unavailable.

**Illustrative AP-mode output (not measured):**

```text
lo0: 127.0.0.1/8 (loopback)
wifi: AP (UP)
  inet 192.168.4.1  netmask 255.255.255.0
  ether xx:xx:xx:xx:xx:xx
```

The actual interface name, status and address depend on the build and connection. `UP` is not proof that an application is listening on a TCP port.

| Field | Meaning |
|---|---|
| `lo0` | Loopback: 127.0.0.1 refers to the Pico W itself |
| `wifi` / `IFACE` | Wi-Fi network interface |
| `AP` / `STA` | Access-point / station role |
| `inet` | Pico W's IPv4 address, **not** the PC's |
| `netmask` | IPv4 network mask |
| `ether` | Interface MAC address, distinct from its IP |

## A working diagnostic sequence

1. Run `ip addr` on PicoNIX to identify its IP.
2. Run `net` to inspect Wi-Fi status, gateway and RSSI ([net reference](NET.md)).
3. Run `ip addr` on the **Linux PC** to find the PC's address.
4. From PicoNIX, run `ping -c 2 PC_IPv4` ([ping guide](PING.md)).
5. An ICMP reply does not prove an HTTP server is running: inspect `ps` and `lsof` separately.

**Further questions:** See [Network observation exercises](../../exercises/README.md#network-observation).

---

[Network index](README.md) | [ping](PING.md) | [日本語](../../../ja/reference/network/IP.md)
