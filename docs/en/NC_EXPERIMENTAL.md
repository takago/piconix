# Experimental nc command (stage 9)

`nc -l 5000`, `nc 192.168.4.2 5000`, `nc -u -l 5000`, `nc -u 192.168.4.2 5000`.

IPv4 literals only. TCP and UDP each allow one socket. Uses polling and nonblocking terminal input. The experimental socket layer has not been tested on a Pico W; do not assume network reliability or compatibility. UDP listening replies to the last received datagram peer.

Limitations: no DNS, multiple clients, half-close, timeout, Ctrl+C handler, or POSIX select. The shell and TTY must support nonblocking reads.
