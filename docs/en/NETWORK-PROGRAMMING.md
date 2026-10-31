# Socket API and network applications

[日本語](../ja/NETWORK-PROGRAMMING.md) | [Network access](NETWORK.md)

## Architecture

On Pico W, lwIP runs on the kernel side and exposes a compact IPv4 socket interface to user processes. `net_socket.c` implements the backend; `net.c` manages Wi-Fi; `net_service_telnet.c` and `net_service_remote_devctl.c` implement built-in kernel services. `nc`, `httpd`, `tcp_echod`, and `udp_echod` are ordinary user programs. Standard Pico boards have no Wi-Fi connectivity.

## Supported API

- TCP: `socket(AF_INET, SOCK_STREAM, 0)`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `close()`.
- UDP: `socket(AF_INET, SOCK_DGRAM, 0)`, `bind()`, `sendto()`, `recvfrom()`, `close()`.
- Connected TCP sockets also support `read()` and `write()`. `send()` and `recv()` are lightweight wrappers that support only flags=0.

**Limitations:** This is not a complete BSD/POSIX implementation. Some operations are nonblocking and return `OS_EAGAIN` when no data is available. Errors are returned as negative `OS_*` codes. TCP `bind()` starts listening, `listen()` is a compatibility operation, and `tcp_status()` checks asynchronous connection completion. DNS, `getsockname()`, and general socket options are not implemented.

## User programs

| Command | Purpose | Default port |
|---|---|---|
| `nc` | TCP/UDP client and server testing | Specified by user |
| `httpd` | HTTP/1.0 server serving `/etc/index.html` | TCP 80 |
| `tcp_echod [PORT]` | TCP echo server, one client at a time | TCP 7 |
| `udp_echod [PORT]` | UDP datagram echo server | UDP 7 |

Start `tcp_echod 1234` on PicoNIX and run `nc 192.168.4.1 1234` on Linux. For UDP, start `udp_echod 1234` and run `printf 'hello\n' | nc -u -w 2 192.168.4.1 1234` on Linux. TCP and UDP have separate port spaces.

Start `httpd` to serve `http://192.168.4.1/`. Edit `/etc/index.html` with `edit` to update the page. Use `lsof` to inspect socket descriptors and local ports.

## Built-in kernel services

- TCP 23: Telnet shell.
- UDP 2000: remote device control with `READ`, `WRITE`, `ENABLE`, and `DISABLE` for permitted GPIO, ADC, PWM, and framebuffer paths.

Example: `printf 'READ /dev/adc/temp\n' | nc -u -w 2 192.168.4.1 2000`. These services do not provide authentication or encryption; use a trusted laboratory network only.

## Exploring the implementation

Kernel network services are separate from user-space socket applications. `trace` can observe system calls and `lsof` can show socket FDs and local ports. Older `NETWORK_STAGE*.md` and experimental notes document development history; this guide describes the current interface.
