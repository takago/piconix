# `ping`: test reachability with ICMP Echo

PicoNIX `ping` sends ICMP Echo Requests to a numeric IPv4 address and reports replies. It runs on **Pico W only** via the exclusive `/dev/net/icmp` device. It is not a general raw-socket implementation and does not support all Linux `ping` flags.

## Usage on PicoNIX

```text
piconix# ping 192.168.4.2
piconix# ping -c 2 192.168.4.2
```

- `-c COUNT`: 1–100 probes; default 4.
- Numeric IPv4 addresses only, no DNS.
- Approximately 1.25 seconds is allowed for each response, with a delay between probes.
- A second simultaneous `ping` may be unable to open the exclusive ICMP device.

Find the **actual PC address** with `ip addr` on Linux and the Pico W address using [PicoNIX `ip`](IP.md). `192.168.4.1` (Pico W) and `.2` (PC) are AP-mode examples, not guaranteed addresses.

## Interpret results

```text
reply from 192.168.4.2: seq=1 time=4 ms
reply from 192.168.4.2: seq=2 time=5 ms
2 transmitted, 2 received
```

**Illustrative output only.** `seq` identifies a probe; `time` is round-trip time in milliseconds. `timeout: seq=N` means no reply was observed for that probe. It does not identify the sole cause: a firewall or ICMP filtering on the PC can be responsible.

## Diagnostic procedure

1. Run `ip addr` and `net` on PicoNIX to check Wi-Fi status.
2. Find the PC address via Linux `ip addr`.
3. On PicoNIX run `ping -c 2 PC_IPv4`.
4. Ping the Pico W's **own Wi-Fi address** to compare local versus PC connectivity.
5. In a second PicoNIX terminal run `tcpdump -c 10`, repeat ping, and look for ICMP `TYPE=8` (request) and `TYPE=0` (reply). See [tcpdump](TCPDUMP.md).

| Failure | First checks |
|---|---|
| `usage` | Numeric address and valid `-c` syntax |
| ICMP device unavailable/busy | Pico versus Pico W, competing `ping` process |
| `send failed` | Wi-Fi state, available network resources |
| `timeout` | Destination address, AP/STA state, firewall, traffic |
| Self-ping works but PC ping fails | PC firewall and AP association |

**Important:** ICMP Echo success is not proof a TCP service is running. Test HTTP, TCP and UDP separately.

---

[Network index](README.md) | [ip](IP.md) | [tcpdump](TCPDUMP.md) | [日本語](../../../ja/reference/network/PING.md)
