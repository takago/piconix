# Socket API and network programming: hands-on guide

This tutorial builds **two ordinary C programs** for Pico W: a UDP echo client and a TCP HTTP client. `nc` is only a diagnostic comparison, not the programming exercise itself. A regular Pico has no Wi-Fi interface. Use an isolated, trusted lab network: the built-in Telnet and remote device service provide neither encryption nor authentication.

## 1. Equipment and data path

You need a Pico W, a USB console, a Linux PC on the same AP/STA network, the Pico SDK, and an ARM toolchain. Start with [Setup](../../../setup/SETUP.md) and [Network connection](../../../reference/network/NETWORK.md).

```text
Pico W: udp_once   → UDP 12345 → PC: udp_echo_host.py
Pico W: tcp_get    → TCP 8000  → PC: python3 -m http.server
C application → syscalls.c → SVC → kernel.c → net_socket.c → lwIP
```

On PicoNIX, run `ip addr` and `net` to confirm its IP state. On the **PC**, run `ip addr` to confirm the PC address (often `192.168.4.2` in AP mode; never assume this value). Stop competing UDP applications: PicoNIX supports only **one user UDP socket at a time**. Allow UDP 12345 and TCP 8000 through the PC firewall for the lab.

## 2. Socket API contracts

| Operation | TCP | UDP | PicoNIX-specific behavior |
|---|---|---|---|
| `socket(AF_INET, ..., 0)` | `SOCK_STREAM` | `SOCK_DGRAM` | Numeric IPv4 only |
| `bind()` | Starts listening | Sets UDP local port | Only `INADDR_ANY` |
| `listen()` | Compatibility check | — | Does not actually start listening |
| `accept(fd,NULL,NULL)` | Accepts | — | `OS_EAGAIN` if no client |
| `connect()` | Begins asynchronous connection | — | Check `tcp_status()` |
| `send()` / `recv()` | Byte stream | — | Partial transfers and `OS_EAGAIN` |
| `sendto()` / `recvfrom()` | — | Datagrams | `flags` must be 0 |
| `close()` | Releases descriptor | Releases descriptor | Always close on exit |

The implementation is **not** a full BSD/POSIX stack. Errors are returned as negative `OS_*` values rather than setting `errno`. Pass numeric IPv4 addresses. Ports in `sockaddr_in` use `htons()`. Refer to [`unistd.h`](../../../../../userland/lib/unistd.h) and the [C API guide](LIBC.md).

## 3. Install and register both programs

From the **Linux PC, in the repository root**:

```sh
cp docs/examples/udp_once.c userland/apps/udp_once.c
cp docs/examples/tcp_get.c userland/apps/tcp_get.c
```

Append `udp_once tcp_get` to the existing `foreach(APP IN ITEMS ...)` list in `CMakeLists.txt`, retaining all old names. Add these declarations alongside other image declarations in `kernel/main.c`:

```c
extern const unsigned char piconix_udp_once_image_start[], piconix_udp_once_image_end[];
extern const unsigned char piconix_tcp_get_image_start[], piconix_tcp_get_image_end[];
```

Add registrations in `main()`:

```c
register_image(49, "udp_once", piconix_udp_once_image_start, piconix_udp_once_image_end);
register_image(50, "tcp_get", piconix_tcp_get_image_start, piconix_tcp_get_image_end);
```

49 and 50 are unused in the checked source version. **Check these IDs again if you have made other changes.** See the [C application build tutorial](PROGRAMMING.md) for the PIC/RAM-slot model and flashing instructions.

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
PICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1" \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

Flash the generated UF2 and use `ls /bin` to confirm both new commands exist.

## 4. UDP client and PC echo server

On the **PC**, start the included Python UDP echo server:

```sh
python3 docs/examples/udp_echo_host.py 12345
```

Its full source code:

```python
"""One-client UDP echo server for the PicoNIX networking tutorial (host PC)."""
import socket
import sys

port = int(sys.argv[1]) if len(sys.argv) > 1 else 12345
if not 1 <= port <= 65535:
    raise SystemExit("port must be 1..65535")
with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
    sock.bind(("0.0.0.0", port))
    print(f"listening UDP {port}", flush=True)
    while True:
        payload, address = sock.recvfrom(512)
        print(f"{address}: {payload!r}", flush=True)
        sock.sendto(payload, address)
```

The **complete Pico W C client** in `docs/examples/udp_once.c` is:

```c
/* Pico W: send one UDP datagram to an echo server on the PC and read the reply. */
#include "unistd.h"
#include "stdio.h"
#include <stdint.h>

static int ipv4(const char *s, uint32_t *result) {
    unsigned char *p = (unsigned char *)result;
    for (int i = 0; i < 4; ++i) {
        unsigned value = 0, digits = 0;
        while (*s >= '0' && *s <= '9') {
            value = value * 10u + (unsigned)(*s++ - '0');
            if (value > 255u || ++digits > 3) return -1;
        }
        if (!digits || (i < 3 ? *s++ != '.' : *s != '\0')) return -1;
        p[i] = (unsigned char)value;
    }
    return 0;
}

static int port_number(const char *s) {
    unsigned n = 0;
    if (!*s) return -1;
    for (; *s; ++s) {
        if (*s < '0' || *s > '9') return -1;
        n = n * 10u + (unsigned)(*s - '0');
        if (n > 65535u) return -1;
    }
    return n ? (int)n : -1;
}

int main(int argc, char **argv) {
    uint32_t address = 0;
    int port = argc == 3 ? port_number(argv[2]) : -1;
    if (argc != 3 || port < 0 || ipv4(argv[1], &address) < 0) {
        puts("usage: udp_once IPv4 PORT");
        return 1;
    }
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { printf("socket: %d\n", fd); return 1; }
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_port = htons((uint16_t)port);
    peer.sin_addr.s_addr = address;
    static const char msg[] = "hello from PicoNIX";
    int sent = sendto(fd, msg, sizeof(msg)-1u, 0,
                      (const struct sockaddr *)&peer, sizeof(peer));
    if (sent != (int)sizeof(msg)-1) {
        printf("sendto: %d\n", sent); close(fd); return 1;
    }
    char reply[80];
    for (int i = 0; i < 200; ++i) {
        int n = recvfrom(fd, reply, sizeof(reply), 0, 0, 0);
        if (n >= 0) {
            printf("received %d bytes: ", n);
            if (n > 0) write(1, reply, (unsigned)n);
            puts(""); close(fd); return 0;
        }
        if (n != OS_EAGAIN) {
            printf("recvfrom: %d\n", n); close(fd); return 1;
        }
        msleep(10);
    }
    puts("timeout (check the PC echo server and its firewall)");
    close(fd);
    return 1;
}
```

On the **PicoNIX shell**, use the PC address determined earlier:

```text
piconix# udp_once 192.168.4.2 12345
received 18 bytes: hello from PicoNIX
```

This output is illustrative, **not a hardware test transcript**. `sendto()` returns the number of bytes sent; `recvfrom()` returns the number of bytes received. `OS_EAGAIN` means no datagram is ready, so the program sleeps briefly and retries. Stop the PC server and retry: a UDP send can succeed even with **no echo reply**. Use `lsof` to inspect descriptors or `tcpdump` to observe packets.

## 5. TCP HTTP client

On the **PC**, start an HTTP server:

```sh
python3 -m http.server 8000 --bind 0.0.0.0
```

The **full C client** in `docs/examples/tcp_get.c` is:

```c
/* Pico W: minimal TCP client for an HTTP server on the host PC. */
#include "unistd.h"
#include "stdio.h"
#include <stdint.h>

static int ipv4(const char *s, uint32_t *result) {
    unsigned char *p = (unsigned char *)result;
    for (int i = 0; i < 4; ++i) {
        unsigned value = 0, digits = 0;
        while (*s >= '0' && *s <= '9') {
            value = value * 10u + (unsigned)(*s++ - '0');
            if (value > 255u || ++digits > 3) return -1;
        }
        if (!digits || (i < 3 ? *s++ != '.' : *s != '\0')) return -1;
        p[i] = (unsigned char)value;
    }
    return 0;
}

static int port_number(const char *s) {
    unsigned n = 0;
    if (!*s) return -1;
    for (; *s; ++s) {
        if (*s < '0' || *s > '9') return -1;
        n = n * 10u + (unsigned)(*s - '0');
        if (n > 65535u) return -1;
    }
    return n ? (int)n : -1;
}

int main(int argc, char **argv) {
    uint32_t address = 0;
    int port = argc == 3 ? port_number(argv[2]) : -1;
    if (argc != 3 || port < 0 || ipv4(argv[1], &address) < 0) {
        puts("usage: tcp_get IPv4 PORT");
        return 1;
    }
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { printf("socket: %d\n", fd); return 1; }
    struct sockaddr_in peer = {0};
    peer.sin_family = AF_INET;
    peer.sin_port = htons((uint16_t)port);
    peer.sin_addr.s_addr = address;
    int r = connect(fd, (const struct sockaddr *)&peer, sizeof(peer));
    if (r < 0) { printf("connect: %d\n", r); close(fd); return 1; }
    int status = 0;
    for (int i = 0; i < 300; ++i) {
        status = tcp_status(fd);
        if (status != 0) break;
        msleep(10);
    }
    if (status != 1) { printf("not connected: %d\n", status); close(fd); return 1; }
    static const char request[] = "GET / HTTP/1.0\r\nHost: piconix-lab\r\n\r\n";
    unsigned off = 0;
    while (off < sizeof(request)-1u) {
        r = send(fd, request + off, sizeof(request)-1u-off, 0);
        if (r == OS_EAGAIN || r == 0) { msleep(10); continue; }
        if (r < 0) { printf("send: %d\n", r); close(fd); return 1; }
        off += (unsigned)r;
    }
    shutdown(fd, SHUT_WR);
    char buf[128];
    int idle = 0;
    for (;;) {
        r = recv(fd, buf, sizeof(buf), 0);
        if (r == 0) break;
        if (r == OS_EAGAIN) {
            if (++idle > 300) { puts("timeout reading response"); close(fd); return 1; }
            msleep(10); continue;
        }
        if (r < 0) { printf("recv: %d\n", r); close(fd); return 1; }
        idle = 0;
        int n = r, used = 0;
        while (used < n) {
            int k = write(1, buf + used, (unsigned)(n-used));
            if (k <= 0) { puts("write failed"); close(fd); return 1; }
            used += k;
        }
    }
    close(fd);
    return 0;
}
```

On PicoNIX:

```text
piconix# tcp_get 192.168.4.2 8000
HTTP/1.0 200 OK
...
```

The response is illustrative. Note that `connect()` initiating a TCP connection does **not** mean it has finished. Wait for `tcp_status() == 1`. `send()` can send fewer bytes than requested; continue with the remainder. A `recv()` of 0 indicates EOF; `OS_EAGAIN` means try later.

## 6. Troubleshooting and experiments

| Symptom | First checks |
|---|---|
| `socket` failure | Pico W firmware; available TCP/UDP socket and task capacity |
| UDP send failure | Numeric IP, destination port, competing UDP owner |
| UDP timeout | PC server running, firewall and return path |
| TCP connection failure | PC listener on TCP 8000, Wi-Fi mode and IP address |
| Repeated `OS_EAGAIN` | Not necessarily an error: sleep briefly and retry, with a timeout |
| Command missing from `/bin` | Both CMake and `register_image`, and the flashed UF2 |

**Exercises:** Explain (1) TCP byte streams versus UDP datagrams, (2) why partial TCP sends require a loop, and (3) the difference between a user-space `httpd` and the kernel Telnet service. Compare the code to `userland/apps/tcp_echod.c`, `udp_echod.c`, and `httpd.c`. For the kernel side, see [Network development](../../network/NETWORK-DEVELOPMENT.md); for command comparison, see [`nc`](../../../reference/network/NC.md).

---

[Development index](../../README.md) | [Network reference](../../../reference/network/README.md) | [日本語](../../../../ja/development/apps/c/NETWORK-PROGRAMMING.md)
