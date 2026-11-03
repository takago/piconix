# nc (netcat) and termination

[Overview](../../literacy/README.md) | [日本語](../../../ja/reference/devices/NC.md) | [Pipes](../../literacy/PIPE.md)

PicoNIX provides a small TCP/UDP `nc` for numeric IPv4 addresses. It does not implement DNS names or the complete option set of desktop netcat implementations.

The examples below assume AP mode: Pico W uses `192.168.4.1`, and the connected Linux PC uses `192.168.4.2` (the single-client DHCP assignment). In STA mode, substitute the actual network addresses.

```sh
nc 192.168.4.2 12345
nc -l 12345
nc -u 192.168.4.2 12345
nc -u -l 12345
```

## Exit on standard-input EOF

Use `-N` for one-shot UDP sends or TCP send-side half-close:

```sh
echo hello | nc -u -N 192.168.4.2 12345
```

On UDP, `-N` terminates `nc` at stdin EOF. On TCP, `-N` uses `shutdown(SHUT_WR)` on stdin EOF to half-close the sending direction while continuing to receive. Full connection termination depends on the peer as well.

UDP has no FIN-style peer-close notification. Closing the UDP netcat on the other machine does not automatically terminate PicoNIX `nc`. Use Ctrl+C for interactive sessions.

## Ctrl+C

Ctrl+C interrupts foreground `nc` on the attached terminal. PicoNIX directly terminates foreground tasks rather than delivering POSIX `SIGINT`. The kernel releases their descriptors and socket resources. Registered foreground pipeline stages are interrupted together.

PicoNIX `nc` does **not** support `-w` timeouts. Do not confuse its options with those of Linux netcat.

## Quick check

Start `nc -u -l 12345` on Linux, then run the `echo hello | nc -u -N ...` command above on PicoNIX, using `192.168.4.2` as the Linux PC address. Verify receipt and that the PicoNIX prompt returns. If the PC receives another address, substitute it. When connecting from Linux to PicoNIX, use `192.168.4.1`.

For file and memory transfer examples, see the [dd guide](../../literacy/DD.md).
