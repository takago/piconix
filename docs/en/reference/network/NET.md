# `net`: Wi-Fi, AP-client and ARP diagnostics

`net` reports **local network state** from `/proc/net/`. It does not itself test reachability or capture packets. `ip` shows a read-only interface summary; use `net` for the more detailed state.

## Commands on PicoNIX

```text
piconix# net
piconix# net -a
piconix# net -r
piconix# net -h
```

| Command | Data | Why use it? |
|---|---|---|
| `net` | `/proc/net/wifi` (mode/status/address/MAC/error history) | Check Wi-Fi connection state |
| `net -a` | `/proc/net/ap` | Check associated AP clients |
| `net -r` | `/proc/net/arp` | Read the current IP/MAC ARP cache |
| `net -h` | Usage | Confirm command syntax |

**Exercise:** Run `net` and check MODE/STATUS. In AP mode, associate the PC and run `net -a`. Make an HTTP request from the PC and then run `net -r`. Finally run [`ping`](PING.md) to test reachability. An empty ARP table is not proof of a network failure, and the values depend on the network.

Use [`ip`](IP.md) for your own IPv4 address, [`ping`](PING.md) for ICMP reachability and [`tcpdump`](TCPDUMP.md) for packet summaries. These features are for Pico W, not regular Pico.

---

[Network index](README.md) | [Wi-Fi details](NETWORK.md) | [日本語](../../../ja/reference/network/NET.md)
