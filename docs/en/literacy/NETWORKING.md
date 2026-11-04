# Networking lab (Pico W)

[日本語](../../ja/literacy/NETWORKING.md) | [Learning path](LEARNING-PATH.md) | [Network reference](../reference/devices/NETWORK.md) | [Monitoring lab](MONITORING.md)

Explore **Wi-Fi connectivity, IP addressing, remote shells, TCP communication, and observing network activity**. This lab **requires Pico W**: a standard Pico with only USB cannot perform the networking exercises. Start with the default access-point (AP) mode on an isolated experimental network.

**Safety:** PicoNIX Telnet and HTTP are unencrypted, and HTTP is unauthenticated. Do not expose them to campus/public networks or the Internet. The AP password is public; do not transmit sensitive information. HTTP APIs can control GPIO/PWM, so connect only safe devices.

## 1. Prepare Pico W

Follow [Setup](../setup/SETUP.md) to build and flash the Pico W firmware using the Linux PC. Code blocks below with `piconix#` run on PicoNIX. Confirm the device-specific AP SSID in its boot output and join it from a PC or phone.

```text
piconix# net
piconix# ip addr
piconix# cat /proc/net/wifi
```

**Check:** Find `MODE`, `STATUS`, and `IP` where available. Explain AP versus station (STA) mode. Fields depend on the connection state. In the usual AP setup, Pico W is `192.168.4.1`; its DHCP service assigns `192.168.4.2` to one client. See [Wi-Fi modes](../reference/devices/WIFI-MODES.md).

## 2. Open a remote shell from the Linux PC

From a **Linux PC terminal already connected to the AP**, run (install the PC Telnet client if needed):

```sh
telnet 192.168.4.1
```

Run `help` and `ps` in the remote session and compare them with the USB console. To quit the PC Telnet client, use `Ctrl+]` and `quit`. **Check:** Why do two different terminal connections see the same OS task state? Read the [network and TTY reference](../reference/devices/NETWORK.md).

## 3. Try HTTP over TCP

The Pico W image normally starts the user-space `httpd` program at boot. Open `http://192.168.4.1/` in the PC browser or run on the **Linux PC**:

```sh
curl http://192.168.4.1/
```

`curl` is a PC command, **not** a PicoNIX command. Use `ps` in PicoNIX to inspect running tasks. HTTP uses TCP port 80, while Telnet uses TCP port 23. **Check:** What does the IP address identify, and what does the port identify? See [HTTP server](../reference/devices/HTTPD.md) and [`nc` reference](../reference/devices/NC.md).

## 4. Monitor the system during communication

On the PicoNIX USB shell:

```text
piconix# ps
piconix# lsof
piconix# net -r
piconix# cat /proc/net/memory
```

**Check:** Verify that `httpd` is a user process, FDs appear in `lsof`, and ARP maps IP addresses to MAC addresses. ARP entries depend on recent traffic; an empty table is not necessarily an error. See the [network reference](../reference/devices/NETWORK.md) for memory and polling diagnostics.

## 5. Optional: TCP client and loopback

On Pico W, `127.0.0.1` addresses the same PicoNIX instance. If `httpd` is running, use the PicoNIX command:

```text
piconix# nc 127.0.0.1 80
```

You can try typing `GET / HTTP/1.0` followed by a blank line to request a page. Terminal newline behavior differs, so this is an optional exercise rather than a guaranteed one-line test. PicoNIX provides both `nc` and a small ICMP `ping` command.

## 6. Observe IPv4 packets with `tcpdump`

`tcpdump` is a **packet observation command** for Pico W. It summarizes the PicoNIX device's own IPv4 TCP, UDP and ICMP traffic. Unlike Linux `tcpdump`, it does not capture Ethernet frames or traffic between other devices, and it cannot write pcap files.

### Command usage

```text
piconix# tcpdump             # Print summaries until Ctrl+C
piconix# tcpdump -c 10       # Stop after 10 packets
piconix# tcpdump -x          # Include data length and up to 16 initial bytes
piconix# tcpdump -x -c 10    # Combine the options
```

`-c COUNT` limits the number of displayed packets. With `-x`, `DATA=` on the first line gives the data length. If data exists, an indented second line shows spaced hexadecimal bytes and their ASCII representation.

### Lab: Observe HTTP traffic

1. On the **PicoNIX USB console**, run `tcpdump -x -c 20` and leave it waiting for packets.

2. From a **Linux PC connected to the PicoNIX AP**, run `curl http://192.168.4.1/` in another terminal. `curl` runs on the PC, **not** on PicoNIX. Alternatively open the same URL in the PC browser. If HTTP does not respond, check `ps` on PicoNIX to see whether `httpd` is running.

3. Inspect the USB console. The following is an **illustrative formatting example**, not a measured capture. Addresses, ports, packet lengths and flags vary.

```text
RX TCP 192.168.4.2:52000 > 192.168.4.1:80 LEN=60 DATA=16 ACK PSH
    DATA:  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 30 0d 0a | GET / HTTP/1.0..|
RX TCP 192.168.4.2:52000 > 192.168.4.1:80 LEN=40 DATA=0 ACK
```

`RX` means received and `TX` means transmitted. `LEN` is the total IPv4 packet length in bytes; `DATA` excludes IP and transport headers. The second line displays **at most the first 16 bytes**. Each hex byte corresponds to the ASCII character on the right; non-printable characters become `.`. The ASCII column remains aligned for shorter samples. There is no second line for `DATA=0`. When the data length exceeds 16, only its beginning is shown.

**Investigate:** Find TCP flags such as `SYN`, `ACK` and `PSH`, and follow connection setup, data transfer and replies. SYN and pure ACK packets normally carry no data. Output may wrap on narrow terminals.

### Lab: Observe ICMP Echo

While `tcpdump -x -c 10` is running on USB, use a **second PicoNIX terminal** (for example Telnet) to run:

```text
piconix# ping -c 2 192.168.4.1
```

This address illustrates the Pico W's own AP-mode IP address. Check the actual address with `ip addr`. To test a PC, use its assigned IP address instead. `TYPE=8` is Echo Request; `TYPE=0` is Echo Reply. Compare the transmitted and received records.

**Notes:** Packets seen before capture starts are not retained. The 16-record ring buffer can overwrite old records during heavy traffic. If the `-c` packet count is not reached, the command keeps waiting; stop it with Ctrl+C. Observe only an isolated lab network you control. See the [network reference](../reference/devices/NETWORK.md).

## Review

- What roles does Pico W play in AP and STA modes?
- Why do Telnet and HTTP use different TCP port numbers?
- What different network properties do `net`, `lsof`, and `/proc/net/memory` show?
- Record the timing of PC communication and PicoNIX observations and explain any changes.

For C-based TCP/UDP clients or servers, continue to [Network programming](../development/apps/c/NETWORK-PROGRAMMING.md). Before remote device control, read the [safety notes and protocol reference](../reference/devices/REMOTE-DEVICE.md).
