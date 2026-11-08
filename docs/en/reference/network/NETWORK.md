# Pico W network access

*Technical reference*

The Pico W build defaults to Wi-Fi AP mode with a small DHCP server; it can also join an existing network in build-time STA mode. The network shell is independent of the USB CDC console. The ordinary Pico has no Wi-Fi support. The current Pico W firmware has also been tested successfully on a regular Pico, but Wi-Fi is unavailable on that hardware.

## Build

With Pico SDK 2.1.1 installed:

```sh
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
LC_ALL=C LANG=C \
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

Set `PICO_TOOLCHAIN_PATH` if using a separately installed toolchain. The UF2 is `build-pico_w/piconix.uf2`. Flash it in BOOTSEL mode. `PICONIX_WIFI_LAYOUT` is obsolete; use `-DPICO_BOARD=pico_w`. For the regular Pico use `-DPICO_BOARD=pico`.

For STA build options and connection status, see [Wi-Fi modes](WIFI-MODES.md).

## Wi-Fi settings

- SSID: `PicoNIX-XXXXXXXX` (device-dependent suffix; check startup output)
- Password: `piconix1234`
- AP IP: `192.168.4.1`
- DHCP client IP: `192.168.4.2` (single-client configuration)

Connect to the AP; the IP address is normally assigned automatically. This AP does not provide Internet access.

## Remote terminal

| Protocol | Port | Command |
|---|---:|---|
| Telnet | TCP 23 | `telnet 192.168.4.1` |

The network shell uses the `/dev/ttyN0` TTY and permits one active network terminal session at a time. USB CDC remains independent.

### Disconnecting

**Telnet:** press `Ctrl + ]`, then type `quit` and Enter at the `telnet>` prompt.

**socat:** when started with `escape=0x1d`, then press `Ctrl + ]`. In raw mode, `Ctrl + C` is normally sent as data rather than terminating socat.

On Linux, install clients as needed (e.g. `sudo apt install telnet socat`). On Windows 10/11, enable the optional Telnet Client feature or run `dism /online /Enable-Feature /FeatureName:TelnetClient` in an elevated shell.

## Notes and security

Telnet and raw TCP are unencrypted. Use only on a trusted experimental network. The AP password is a published fixed value; do not send secrets. Terminal behavior can vary with the Telnet client. For current Wi-Fi and DHCP diagnostics, see [Wi-Fi modes](WIFI-MODES.md) and [network status](NET.md). Earlier investigations are retained in Git history.

## TCP/UDP applications and remote device control

Alongside Telnet (TCP 23), Pico W supports user-space `nc`, `httpd` (TCP 80), `tcp_echod`, and `udp_echod`. UDP 2000 offers remote access to permitted device paths. See [Remote device control](REMOTE-DEVICE.md) for the UDP protocol and [Socket API and network applications](../../development/apps/c/NETWORK-PROGRAMMING.md) for user-space sockets.

Wi-Fi STA reassociation can be requested with `echo restart > /dev/net/wifi`. The request is processed in the network poll loop; an active remote session may disconnect. `/dev/ttyN0` remains the terminal device.

## Local loopback (Pico W)

On Pico W, in both AP and STA modes, lwIP's `lo0` interface handles local
TCP/UDP traffic to `127.0.0.1`. Wi-Fi association is not required, but the
network stack must be initialized. For example, start `httpd &` and then
connect with `nc 127.0.0.1 80`.

PicoNIX uses lwIP's polling raw API. Loopback packets are delivered from
the idle task's `net_poll()` instead of reentering `tcp_input()` recursively;
responses may be processed during the next poll. The regular Pico build does
not link lwIP and does not provide loopback. The loopback queue is limited to
8 pbufs; inspect `cat /proc/net/memory` during load testing.

Packets addressed to the Wi-Fi interface’s own IPv4 address are also queued
for local delivery instead of being sent over the wireless link. Thus, in AP
mode, `ping -c 2 192.168.4.1` can test the device itself, and the same local
routing applies to TCP and UDP. In STA mode, use the interface’s assigned
address. This does not create a second network interface or a new syscall.

## TCP receive and connection cleanup

Up to three user UDP sockets may be open simultaneously. Each has its own lwIP PCB and 512-byte receive buffer. For example, `scroll -u 1234 &` can run alongside `nc -u 127.0.0.1 1234`. Each socket queues at most one incoming datagram. The lwIP UDP PCB pool has eight entries shared with DHCP, mDNS and remote device control; available PCB capacity depends on enabled services. Hardware validation is pending.

The user TCP socket backend has four fixed slots. Each receive buffer is sized to
`TCP_WND` (currently 1,072 bytes, or two 536-byte segments), so it can hold an
entire advertised receive window. A callback returns `ERR_MEM` **without**
freeing the pbuf when the buffer is temporarily full; after the application
reads data, lwIP can retry delivery. `tcp_recved()` is called only for bytes
read by the application. On a Telnet disconnect, an aborted PCB is signalled
with `ERR_ABRT` to prevent lwIP from using an already freed PCB.

The receive-buffer change adds 48 bytes per slot (192 bytes total) of static
RAM compared with the previous 1,024-byte buffers. Verify the kernel RAM limit
with `tools/build_sdk.sh` and its ELF layout checks after integrating this
change. These safeguards do not yet establish that the earlier
`tcp_receive: valid queue length` assertion has been completely eliminated.

TCP `accept()` and `read()`, and UDP `recvfrom()`, block the task by default when not ready. With `O_NONBLOCK`, they return `OS_EAGAIN` instead. Sending and asynchronous `connect()` have different semantics, including partial sends or temporary unavailability. UDP buffers just one datagram; an undersized receive buffer returns `OS_EINVAL` without consuming it. See the [socket programming guide](../../development/apps/c/NETWORK-PROGRAMMING.md).

## TCP transmission and half-close

Each user TCP write is bounded by `TCP_MSS` (536 bytes) and the available `tcp_sndbuf()`. If `tcp_write()` returns `ERR_MEM`, nothing from that attempt is counted as queued and the caller can retry; other errors are not mistaken for transient backpressure. Once `tcp_write()` succeeds, data is queued even if the subsequent `tcp_output()` fails, so the caller must not enqueue those bytes again.

After successful `shutdown(SHUT_WR)`, writes fail for that socket. If `tcp_close()` temporarily returns `ERR_MEM`, the backend retries from its TCP poll callback rather than closing during ACK processing. Late pbufs received by a detached closing PCB are consumed, and a callback that frees a pbuf returns `ERR_OK`.

**About the prior assertion:** `tcp_receive: valid queue length` detects a nonzero send-queue count despite both `unacked` and `unsent` lists being empty during ACK processing. Raw-API reentrancy, send-queue ownership and memory corruption remain under investigation; these changes do not prove the underlying cause has been eliminated.

## Polling and Wi-Fi stability

PicoNIX uses the polling-based CYW43/lwIP integration. `net_poll()` runs in the idle task, **not** in the 1 ms timer interrupt. It calls `cyw43_arch_poll()` and runs the network services only when at least **250 µs** have elapsed since the previous network poll. This interval applies to both STA and AP modes; it is a **minimum interval**, not a guaranteed 250 µs schedule. Other tasks can delay the idle task.

The setting is defined by `NET_POLL_INTERVAL_US` in `kernel/net.c`. Its history and rationale are:

- Initially, polling was unrestricted, with roughly 100,000 calls/s observed during idle time.
- A 1 ms minimum interval reduced calls to at most approximately 1,000/s, but interactive Telnet responsiveness appeared worse.
- 500 µs (at most approximately 2,000 calls/s) improved perceived responsiveness.
- 250 µs (at most approximately 4,000 calls/s) is the current setting, chosen for responsiveness; its performance and long-term stability have not been systematically measured.

For STA, PicoNIX explicitly disables CYW43 Wi-Fi power saving using `CYW43_NONE_PM`. The idle task stack is 4 KiB. `/proc/net/wifi` reports `POLL_COUNT` (cumulative **executed** polls), `POLL_MAX_GAP_MS` (largest observed gap), `PM_ERROR` (STA power-management setup result), and `IDLE_STACK` (estimated stack usage). `POLL_COUNT` does not reset when the STA connection is restarted and does **not** count received packets.

To request a manual STA reconnect, use `echo restart > /dev/net/wifi`. This does not restart the OS or reset the network poll counter; an existing Telnet session may disconnect.

## Network memory diagnostics

On Pico W, `cat /proc/net/memory` reports `LWIP_HEAP` (current / peak /
capacity in bytes), `LWIP_HEAP_ERRORS` (cumulative allocation failures),
`PBUF_POOL` (current / peak / number of entries), `PBUF_POOL_ERRORS`
(cumulative pool allocation failures), and `IDLE_STACK` (estimated high-water
bytes / capacity). The pbuf pool has eight entries of 512 bytes each; it is
still allocated in kernel SRAM. This is **not** a measure of all CYW43 memory.
The 10-KiB dynamic heap and the 4-KiB idle stack reside in the 16-KiB network
bank with a 2-KiB safety gap. Counters reset at boot and a zero-error count
does not guarantee a stable TCP connection. Stress-test AP and STA builds.

## Packet observation and ICMP Echo

Pico W provides two small educational network devices. `/dev/net/packet` is a read-only stream of **fixed-size packet records**, not raw Ethernet frames. The kernel records IPv4 TCP/UDP/ICMP header summaries and at most 16 initial payload bytes; an internal 16-record ring buffer overwrites old records. Each reader starts with new traffic; `read()` returns `OS_EAGAIN` when nothing is available. It does not capture ARP, other clients’ traffic, or packets bypassing the local IPv4 stack. `tcpdump` prints records as text; it does not create pcap files.

`/dev/net/icmp` is an **exclusive** read/write interface for ICMP Echo Request/Reply. The user `ping` command sends an Echo Request, polls for its reply, and reports elapsed time or a timeout. It is not a generic RAW socket and accepts numeric IPv4 addresses only. Both devices are available on **Pico W only**. The record layouts are documented in [`include/piconix_net_capture.h`](../../../../include/piconix_net_capture.h) in the repository. All other network applications continue using existing TCP/UDP sockets.

```text
piconix# tcpdump -c 10
piconix# ping -c 2 192.168.4.2
```

`tcpdump [-x] [-c COUNT]` runs on Pico W and the options may be combined. Start `tcpdump -x -c 10` on the USB console, then generate traffic using `curl http://192.168.4.1/` on a PC or `ping` on a second PicoNIX terminal. For step-by-step instructions and sample output, see the [Network reference](NETWORK.md). `LEN` is the IPv4 packet length; `DATA` is the transport payload length (for a first TCP/UDP fragment). `tcpdump` normally displays one summary line per packet. With `-x`, the first line includes `DATA=` (transport payload length); when data is present, an indented second line labelled `DATA:` shows the first 16 bytes in spaced hexadecimal with an aligned ASCII column (`|...|`), as in `hexdump`. Packets without payload (for example, TCP SYN/ACK) show `DATA=0` without a sample. ICMP uses TYPE to distinguish Echo Request (8) and Reply (0). Commands that wait for traffic can be stopped using the shell’s Ctrl+C mechanism. Packets arriving before capture starts are intentionally not stored. Traffic inspection can expose application data: use only a trusted lab network.

### Practical command guides

- [`ip`](IP.md): interfaces and IPv4 address
- [`ping`](PING.md): ICMP reachability and failures
- [`tcpdump`](TCPDUMP.md): TCP/UDP/ICMP packet summaries
- [`net`](NET.md): Wi-Fi, AP clients and ARP
- [C networking lab](../../development/apps/c/NETWORK-PROGRAMMING.md): complete code and build steps


---

**Related links:** [Reference index](../README.md) | [Learning roadmap](../../ROADMAP.md) | [日本語](../../../ja/reference/network/NETWORK.md) | [Setup](../../setup/SETUP.md)
