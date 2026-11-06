# `tcpdump`: observe Pico W's own IPv4 traffic

PicoNIX `tcpdump` reads **packet summaries** from `/dev/net/packet`. This is not Linux tcpdump: it does not capture whole Ethernet frames, other devices' traffic, ARP, or pcap files. It reports TCP, UDP and ICMP traffic in which the Pico W participates.

## PicoNIX commands

```text
piconix# tcpdump
piconix# tcpdump -c 10
piconix# tcpdump -x -c 20
```

| Flag | Effect |
|---|---|
| none | Wait and display until Ctrl+C |
| `-c COUNT` | Stop after COUNT records (positive integer) |
| `-x` | Also show data length and the first up to 16 bytes as hex/ASCII |

An empty screen may simply mean that no packets are being sent to/from the Pico W. Records are held in a **16-entry ring buffer**; busy traffic can overwrite old entries. ARP and traffic between unrelated devices are not captured.

## First example: capture HTTP

1. Connect the Pico W USB console and check `ip addr`.
2. On the **PicoNIX USB shell**, start `tcpdump -x -c 20`.
3. On the **Linux PC**, run `curl http://192.168.4.1/` (replace with the actual Pico W IP).
4. Observe `RX TCP` and `TX TCP` on the PicoNIX screen. If fewer than 20 records arrive, use Ctrl+C to stop.

**Illustrative output (not hardware capture):**

```text
RX TCP 192.168.4.2:52000 > 192.168.4.1:80 LEN=60 DATA=16 ACK PSH
    DATA:  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 30 0d 0a | GET / HTTP/1.0..|
```

| Field | Meaning |
|---|---|
| `RX` / `TX` | Direction from the Pico W's perspective |
| `TCP` / `UDP` / `ICMP` | IP transport/control protocol |
| `src:port > dst:port` | Endpoints; ICMP has no port fields |
| `LEN=` | Total IPv4 packet length |
| `DATA=` | TCP/UDP data length (with `-x`) |
| `SYN` / `ACK` / `FIN` / `RST` / `PSH` | TCP connection/control flags |
| `TYPE=8` / `TYPE=0` | ICMP Echo Request / Reply |
| second `DATA:` line | Up to 16 initial data bytes, not entire payload |

## Compare protocols

- **ICMP:** In a second PicoNIX shell, run `ping -c 2 192.168.4.1` (replace with the actual Pico W address) and inspect `TYPE=8/0`. See [ping](PING.md).
- **TCP:** Run PC-side `curl` to observe SYN, ACK and FIN.
- **UDP:** Run the included `udp_echo_host.py` on the PC and `udp_once` on the Pico W, then inspect the UDP packets. See [socket programming](../../development/apps/c/NETWORK-PROGRAMMING.md).

**Privacy:** `-x` may expose application data. Use only on your own trusted lab network, especially with cleartext Telnet.

---

[Network index](README.md) | [ping](PING.md) | [Socket programming](../../development/apps/c/NETWORK-PROGRAMMING.md) | [日本語](../../../ja/reference/network/TCPDUMP.md)
