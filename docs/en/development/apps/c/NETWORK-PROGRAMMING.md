# Socket API and network applications

> **Document type: DEVELOPMENT GUIDE (not a textbook chapter).** [Development guide index](../../README.md) | [Part C textbook](../../../learn/C/README.md).

[日本語](../../../../ja/development/apps/c/NETWORK-PROGRAMMING.md) | [Network access](../../../reference/network/NETWORK.md)

## Architecture

This guide covers the **IPv4 socket API exposed to user-space C applications** and practical examples. Pico W runs lwIP in the kernel; `nc`, `httpd`, `tcp_echod`, and `udp_echod` are user programs. Ordinary Pico boards have no Wi-Fi. TCP/UDP loopback to `127.0.0.1` is available on Pico W.

Kernel-side `net.c`, `net_socket.c`, Telnet, and UDP device-service implementation details now belong in the [kernel networking development guide](../../network/NETWORK-DEVELOPMENT.md). For connection rules and memory limits, consult [Network access](../../../reference/network/NETWORK.md) and [Memory layout](../../../reference/MEMORY-LAYOUT.md).

## Supported API

- TCP: `socket(AF_INET, SOCK_STREAM, 0)`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `close()`.
- UDP: `socket(AF_INET, SOCK_DGRAM, 0)`, `bind()`, `sendto()`, `recvfrom()`, `close()`.
- Connected TCP sockets also support `read()` and `write()`. `send()` and `recv()` are lightweight wrappers that support only flags=0.

**Limitations:** This is not a complete BSD/POSIX implementation. Some operations are nonblocking and return `OS_EAGAIN` when no data is available. Errors are returned as negative `OS_*` codes. TCP `bind()` starts listening, `listen()` is a compatibility operation, and `tcp_status()` checks asynchronous connection completion. DNS, `getsockname()`, and general socket options are not implemented.

## Exact limitations of the BSD-style wrappers

In `userland/lib/syscalls.c`, `listen(fd, backlog)` returns `OS_EINVAL` for a negative backlog, but otherwise returns 0 **without validating the FD**. TCP listening actually starts in `bind()`; a successful `listen()` call must not be used to establish whether a socket is listening.

`accept(fd, addr, len)` does not support obtaining the peer address: if either `addr` or `len` is non-NULL, it returns `OS_EINVAL`. Use `accept(fd, NULL, NULL)` with the current interface.

For `recvfrom()`, if `from` is non-NULL, `fromlen` must also be non-NULL and the incoming `*fromlen` must be at least `sizeof(struct sockaddr_in)`. On success the wrapper fills in the IPv4 sender information and the actual structure size. Use `from == NULL` to skip retrieving the sender address.

The `flags` argument to `send()`, `recv()`, `sendto()`, and `recvfrom()` must be zero. These functions return negative `OS_*` codes directly rather than setting POSIX `errno`. Recheck this section if the socket wrappers change.

## User programs

| Command | Purpose | Default port |
|---|---|---|
| `nc` | TCP/UDP client and server testing | Specified by user |
| `httpd` | HTTP/1.0 server serving `/etc/index.html` | TCP 80 |
| `tcp_echod [PORT]` | TCP echo server, one client at a time | TCP 7 |
| `udp_echod [PORT]` | UDP datagram echo server | UDP 7 |

Start `tcp_echod 1234` on PicoNIX and run `nc 192.168.4.1 1234` on Linux. For UDP, start `udp_echod 1234` and run `printf 'hello\n' | nc -u -w 2 192.168.4.1 1234` on Linux. TCP and UDP have separate port spaces.

Start `httpd` to serve `http://192.168.4.1/`. To change the page, run `edit /etc/index.html` and save it. Files under `/etc` are RAM-backed, so changes are lost on reboot. Use `lsof` to inspect socket descriptors and local ports.

## Built-in kernel services

- TCP 23: Telnet shell.
- UDP 2000: remote device control with `READ`, `WRITE`, `ENABLE`, and `DISABLE` for permitted GPIO, ADC, PWM, and framebuffer paths.

Example: `printf 'READ /dev/adc/temp\n' | nc -u -w 2 192.168.4.1 2000`. These services do not provide authentication or encryption; use a trusted laboratory network only.

## Exploring the implementation

Kernel network services are separate from user-space socket applications. `trace` can observe system calls and `lsof` can show socket FDs and local ports. Older `NETWORK_STAGE*.md` and experimental notes document development history; this guide describes the current interface.

