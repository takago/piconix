# TCP/UDP Socket Programming with Tiny Python

*Development guide — Pico W only*

[Python fundamentals](PYTHON.md) | [Device programming](DEVICE-PROGRAMMING.md) | [Network reference](../../../reference/network/NETWORK.md) | [日本語](../../../../ja/development/apps/python/SOCKET-PROGRAMMING.md)

PicoNIX does not implement CPython's `socket` module. Instead, its Tiny Python uses a dedicated API with **integer file descriptors (FDs)**. Standard Pico boards lack Wi-Fi sockets; use Pico W for socket operations. Traffic is unencrypted, so use only a trusted isolated network.

## Preparation

Confirm the Pico W Wi-Fi connection. Use `cat /proc/net/wifi` to find its IP address. In AP mode the Pico W commonly uses `192.168.4.1`. Create scripts using `edit /tmp/demo.py` and run them with `python /tmp/demo.py`. Avoid conflicts with existing service ports.

## API overview

| Tiny Python API | Meaning |
|---|---|
| `socket(1)` / `socket(2)` | Create a TCP / UDP descriptor |
| `connect(fd, "127.0.0.1", 80)` | Initiate a TCP connection; completion requires a separate check |
| `state = status(fd)` | TCP state: 0=in progress, 1=established, negative=failed, etc. |
| `listen(fd, 9000)` | Start a TCP listener |
| `client = accept(fd)` | Accept an incoming TCP connection; blocks by default |
| `bind(fd, 9000)` | Bind the UDP receive port |
| `n = send(fd, data)` | TCP send, which can be partial or temporarily unavailable |
| `data = recv(fd, 64)` | TCP receive, maximum 64 bytes |
| `n = sendto(fd, data, "192.168.4.2", 9000)` | Send a UDP datagram |
| `data = recvfrom(fd, 64)` | Receive a UDP datagram |
| `peerport()` / `sendto(fd, data, "peer", 0)` | Read the most recent UDP source port / reply to that source |
| `close(fd)` | Release an FD |

`connect()` and `sendto()` require a numeric IPv4 address as a string literal. There is no Python hostname-resolution API. `"127.0.0.1"` means the loopback path within the same Pico W.

### Interpreting return values

`accept()`, `recv()` and `recvfrom()` **block by default** until a connection or data arrives. Python also supports `fcntl(fd, F_GETFL, 0)` and `fcntl(fd, F_SETFL, flags | O_NONBLOCK)`. On nonblocking sockets an unavailable receive produces `OS_EAGAIN`, which the Python receive functions convert to integer `-1`; its receive functions return integer `-1` on actual failures. Successful receives return byte sequences; `[]` represents TCP EOF or a zero-length UDP datagram. Check `data != -1` before `len()` or `write()`. Use `status(fd)` to inspect TCP state.

`accept()` waits for a connection by default; check `client >= 0` for actual errors. `send()` may transmit only part of the payload or be temporarily unable to send. Each call is limited to 64 bytes. Payloads can be literals, string variables, list/`bytes` variables, or `str(integer_expression)`. `recv()` and `recvfrom()` **must be used in assignment statements**.

## 1. TCP client: HTTP server on the same Pico W

First start the user-space `httpd` on Pico W with `httpd &` in the shell. Create `/tmp/http.py`:

```python
fd = socket(1)
if fd < 0:
    print("socket failed")
    exit()
if connect(fd, "127.0.0.1", 80) != 0:
    print("connect failed")
    close(fd)
    exit()
state = 0
while state == 0:
    state = status(fd)
    msleep(10)
if state != 1:
    print("connection failed")
    close(fd)
    exit()
send(fd, "GET / HTTP/1.0\r\n\r\n")
msleep(100)
data = recv(fd, 64)
if data != -1:
    write(1, data)
close(fd)
```

Run `python /tmp/http.py` to display **up to the first 64 bytes** of the HTTP response. Depending on timing, the response may be empty. This is not a complete HTTP client and does not retry sends or collect the complete response. The interpreter imposes a maximum iteration count on each `while` loop.

## 2. TCP server: temperature reading

The existing sample [`temp_tcp.py`](../../../../../userland/examples/python/temp_tcp.py) listens on TCP port 9000 and uses `open("/dev/adc/temp", 0)` plus `read()` to send an on-chip temperature reading, then closes the accepted connection. Enter or copy the source into `/tmp/temp_tcp.py`, then run:

```sh
python /tmp/temp_tcp.py
```

Test from a PC, replacing `PICO_IP` with the actual Pico W address:

```bash
nc PICO_IP 9000
```

Keep the listening FD and individually `close()` each client descriptor returned by `accept()`. This minimal educational sample sends one short response without retrying a partial send.

## 3. UDP server: reply to the sender

The existing [`temp_udp.py`](../../../../../userland/examples/python/temp_udp.py) receives on UDP port 9000 and replies with the temperature. After `recvfrom()`, the special `"peer"` address and port `0` designate the most recently received sender.

```python
fd = socket(2)
if fd < 0:
    print("socket failed")
    exit()
if bind(fd, 9000) != 0:
    print("bind failed")
    close(fd)
    exit()
while True:
    data = recvfrom(fd, 64)
    if data != -1:
        if len(data) > 0:
            sendto(fd, "OK\n", "peer", 0)
close(fd)
```

Save this as `/tmp/udp.py` and run `python /tmp/udp.py`. From a separate PC:

```bash
printf 'hello' | nc -u -w 2 PICO_IP 9000
```

The expected response is `OK`. UDP has a one-datagram receive buffer. If a read request is smaller than the pending datagram, the kernel reports an error while preserving it. Tiny Python reads at most 64 bytes, so **do not send requests longer than 64 bytes**. `"peer"` refers to only the last received sender and is overwritten by subsequent receives.

## 4. Educational implementation limits

- Each Python `while` loop is limited to 10,000 iterations, so these examples are not designed as permanent background services.
- TCP has a limited number of management slots; UDP permits up to three user sockets system-wide. See the [network reference](../../../reference/network/NETWORK.md). Each socket holds at most one pending received datagram.
- `recv()` and `recvfrom()` return `-1` for non-blocking unavailability or failures, and `[]` for a zero-byte result. `-1` does not distinguish temporary unavailability from a fatal error; check `status(fd)` where applicable.
- Splitting larger messages, retrying partial TCP sends, recognizing disconnections and enforcing timeouts are application responsibilities.
- These functions are not compatible with the standard Python `socket` module. `import socket` is not supported.

**See also:** [Python fundamentals](PYTHON.md) / [C networking API](../c/NETWORK-PROGRAMMING.md)

**Initialization failures:** The bundled TCP/UDP examples report failed `socket()`, `listen()`, or `bind()` calls, close any allocated FD, and call `exit()`. A failed client connection or individual sensor read does not terminate the server. `exit()` returns status 0.


**TCP/UDP temperature servers:** `accept()` and `recvfrom()` block by default. The examples therefore do not poll with `msleep()`; the task blocks while waiting for input.

### `fcntl()` and nonblocking I/O

`fcntl(fd, cmd, value)` takes the same three arguments as the PicoNIX C API. Constants include `F_GETFL`, `F_SETFL`, and `O_NONBLOCK`. Only `O_NONBLOCK` can be changed via `F_SETFL`, and only on supported TTY and TCP/UDP socket descriptors.

```python
flags = fcntl(fd, F_GETFL, 0)
if flags >= 0:
    result = fcntl(fd, F_SETFL, flags | O_NONBLOCK)
```

On nonblocking sockets, `recv()`, `recvfrom()`, and `accept()` return `-1` when no data or connection is ready. TTY `read()` instead returns `[]` when no input is ready; semantics depend on the descriptor type. This is a PicoNIX built-in, not CPython fcntl module.
