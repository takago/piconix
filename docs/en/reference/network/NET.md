# net: Network diagnostics

> **Document type: TECHNICAL REFERENCE.** Use this page to look up specifications. [Reference index](../README.md) | [Textbook](../../learn/README.md).

`net` is a user-space command to inspect Pico W network status.

```sh
net       # Basic Wi-Fi/IP/DHCP information (/proc/net/wifi)
net -a    # Associated AP stations (/proc/net/ap, MAC addresses)
net -r    # ARP cache (/proc/net/arp, IP and MAC)
```

STA mode shows `DHCP: Client`; AP mode reports whether the DHCP server started. `net -a` is AP-only, while `net -r` works in AP and STA. ARP entries represent recently resolved peers, not a definitive list of connected stations. The PicoNIX AP DHCP server supports one lease only.

The kernel polls the CYW43 association list about once per second in the network polling context rather than making driver calls in system calls. Both new proc nodes are read-only.
