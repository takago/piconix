# Experimental socket API (stage 8)

This snapshot fixes duplicate Pico (non-Wi-Fi) socket stub definitions and exposes the existing TCP syscalls to freestanding userland code. It does **not** yet provide a complete `nc` command or a BSD-compatible socket API.

Available userland calls: `socket(AF_INET, SOCK_STREAM, 0)`, `tcp_listen(fd, port)`, `tcp_accept(fd)`, `tcp_connect(fd, &endpoint)`, `tcp_status(fd)`, `read`, `write`, `close`; and UDP `bind_udp`, `sendto_udp`, `recvfrom_udp`.

The TCP accept and connect APIs are experimental/nonblocking. There is only one global TCP connection slot; no concurrent TCP sessions. Existing telnet and UDP device services are separate. Do not deploy without Pico W hardware verification.
