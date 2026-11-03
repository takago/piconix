# Experimental nc command (stage 9)

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/literacy/README.md), [Internals](../../en/reference/INTERNAL.md), [Pipes](../../en/literacy/PIPE.md), and [Network programming](../../en/development/apps/c/NETWORK-PROGRAMMING.md).


`nc -l 5000`, `nc 192.168.4.2 5000`, `nc -u -l 5000`, `nc -u 192.168.4.2 5000`.

IPv4 literals only. TCP and UDP each allow one socket. Uses polling and nonblocking terminal input. The experimental socket layer has not been tested on a Pico W; do not assume network reliability or compatibility. UDP listening replies to the last received datagram peer.

Limitations: no DNS, multiple clients, half-close, timeout, Ctrl+C handler, or POSIX select. The shell and TTY must support nonblocking reads.
