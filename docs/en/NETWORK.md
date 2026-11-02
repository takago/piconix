# Pico W network access

[日本語](../ja/NETWORK.md) | [Setup](SETUP.md)

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

Telnet and raw TCP are unencrypted. Use only on a trusted experimental network. The AP password is a published fixed value; do not send secrets. Terminal behavior can vary with the Telnet client. For maintenance notes about DHCP failures, see the [historical DHCP investigation](../history/ja/NETWORK-DHCP-NOTES.md).

## TCP/UDP applications and remote device control

Alongside Telnet (TCP 23), Pico W supports user-space `nc`, `httpd` (TCP 80), `tcp_echod`, and `udp_echod`. UDP 2000 offers remote access to permitted device paths. See [Remote device control](REMOTE-DEVICE.md) for the UDP protocol and [Socket API and network applications](NETWORK-PROGRAMMING.md) for user-space sockets.

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

## TCP receive and connection cleanup

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

## Network memory diagnostics (stage 9)

On Pico W, `cat /proc/net/memory` reports `LWIP_HEAP` (current / peak /
capacity in bytes), `LWIP_HEAP_ERRORS` (cumulative allocation failures),
`PBUF_POOL` (current / peak / number of entries), `PBUF_POOL_ERRORS`
(cumulative pool allocation failures), and `IDLE_STACK` (estimated high-water
bytes / capacity). The pbuf pool has eight entries of 512 bytes each; it is
still allocated in kernel SRAM. This is **not** a measure of all CYW43 memory.
The 10-KiB dynamic heap and the 4-KiB idle stack reside in the 16-KiB network
bank with a 2-KiB safety gap. Counters reset at boot and a zero-error count
does not guarantee a stable TCP connection. Stress-test AP and STA builds.
