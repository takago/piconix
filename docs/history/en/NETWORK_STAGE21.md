# Stage 21: send/recv and echo daemons

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/README.md), [Internals](../../en/INTERNAL.md), [Pipes](../../en/PIPE.md), and [Network programming](../../en/NETWORK-PROGRAMMING.md).


- Added `send()` and `recv()` for connected sockets, with flags restricted to zero. They wrap existing `write()` and `read()` syscalls; no kernel syscall added.
- `httpd` uses `recv()` and `send()` on TCP client sockets, retaining `read()` for `/etc/index.html`.
- `nc` uses `recv()` and `send()` on TCP sockets and `read()`/`write()` for the console; UDP retains `recvfrom()`/`sendto()`.
- Added user programs `tcp_echod [PORT]` and `udp_echod [PORT]`, defaulting to RFC 862 echo port 7. Both run as user programs, not kernel services.
- `tcp_echod` serves one TCP client at a time; `udp_echod` echoes one datagram per receive.

Check with `nc <PICO_IP> 7` and `printf 'test\n' | nc -u -w 2 <PICO_IP> 7` after launching the respective program.

Note: PicoNIX socket calls return negative OS errors directly and do not support nonzero flags.
