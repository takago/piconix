# Experimental UDP socket syscall bridge (stage 3)

This stage exposes the existing UDP transport through four syscall entry points: `socket(AF_INET, SOCK_DGRAM, 0)`, `bind_udp(fd, port)`, `sendto_udp(fd, &packet)`, and `recvfrom_udp(fd, &packet)`. `close(fd)` releases the socket. The socket is owned by one task; task exit also releases it. `dup2` cannot duplicate or overwrite the UDP socket descriptor.

The current API is **not** a complete BSD socket interface. There is only one global UDP socket and one queued datagram of at most 512 bytes. The packet struct in `include/piconix.h` uses lwIP's network-order 32-bit address representation. `recvfrom_udp` returns `OS_EAGAIN` when no packet is queued. No blocking wait is implemented yet.

TCP sockets, `nc`, and HTTP are not included. This code requires Pico W build and hardware verification before being treated as stable.
