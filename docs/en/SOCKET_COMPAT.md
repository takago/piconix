> Historical implementation notes. For the current overview see [Socket API and network applications](NETWORK-PROGRAMMING.md).

# Minimal BSD-style socket compatibility API

PicoNIX provides a small IPv4 socket interface in `userland/unistd.h`,
implemented as wrappers in `userland/syscalls.c`. Existing kernel syscalls
and legacy `tcp_*` / `*_udp` entry points are retained.

- `socket(AF_INET, SOCK_STREAM/SOCK_DGRAM, 0)`
- `bind(fd, sockaddr_in*, sizeof(sockaddr_in))` for wildcard local addresses
- `listen(fd, backlog)` (compatibility no-op; TCP `bind` starts listening)
- `accept(fd, NULL, NULL)` (nonblocking; returns `OS_EAGAIN` if pending)
- `connect(fd, sockaddr_in*, sizeof(sockaddr_in))` (starts async TCP connect;
  use `tcp_status(fd)` to poll for completion)
- `sendto` and `recvfrom` for UDP (nonblocking; no data: `OS_EAGAIN`)
- `read`, `write`, `send(flags=0)`, `recv(flags=0)`, `close` for established TCP connections

`sin_port` and `sin_addr.s_addr` use network byte order; use `htons`/`ntohs`
for ports. IPv4 address bytes can be parsed into `sin_addr.s_addr` as in
`userland/nc.c`. No DNS, errno, socket options, `getsockname`, or full
POSIX blocking semantics are provided. Errors are returned directly as
negative `OS_*` codes. TCP `bind()` currently starts listening; `listen()`
does not change the kernel state or validate the descriptor. `accept()` does
not yet return a peer address. The implementation supports only the small
socket limits of the existing kernel backend.
