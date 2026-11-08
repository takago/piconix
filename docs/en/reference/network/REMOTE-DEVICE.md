# Remote device control (UDP port 2000)

*Technical reference*

Pico W runs a **kernel-side UDP service** on port **2000** for a small allowlist of device operations. It is different from the user-space TCP/UDP socket API. The service has **no authentication, encryption, request identifiers, or retransmission**. Use it only on an isolated, trusted experimental network; do not expose this port to the Internet. Requests can change GPIO outputs, PWM settings, and the LED matrix. Never rely on the network connection alone to stop hazardous actuators.

## Connect and try it

In default AP mode, join the PicoNIX access point and use `192.168.4.1`. In STA mode, replace it with the Pico W's assigned address. From a host PC with Python 3:

```sh
python3 host/piconixctl.py read /dev/adc/temp
python3 host/piconixctl.py read /dev/gpio/16
python3 host/piconixctl.py enable /dev/fb
python3 host/piconixctl.py write /dev/fb 183c7effff7e3c18 --hex
python3 host/piconixctl.py disable /dev/fb
```

The GPIO read may be unavailable if the selected pin is reserved. To use a different address, add `--host ADDRESS` **before** the command. `host/piconixctl.py` also accepts `--timeout` and `--port`. The 8-byte framebuffer uses binary data; `--hex` converts hexadecimal input/output. For a continuous animation and ADC demo, run `python3 host/demo_udp.py --interval 0.2`. Neither utility automatically retries timed-out requests.

## On-wire protocol

Each UDP datagram contains one request. The maximum request datagram is **128 bytes**. The server sends a single response to the source address and port:

| Request (ASCII prefix) | Meaning |
|---|---|
| `READ <path>` | Read the supported device; result bytes follow `OK\n` |
| `WRITE <path>\n<payload>` | Write raw payload bytes (a space can also separate the path and payload) |
| `ENABLE <path>` | Enable the framebuffer or set an allowed GPIO pin to output |
| `DISABLE <path>` | Disable the framebuffer or set an allowed GPIO pin to input |

A successful reply is `OK\n` followed by device bytes for `READ`, or just `OK\n` for other operations. A rejected request receives exactly `ERR`. A lost or ignored datagram produces no response, so the client must handle a timeout. The implementation accepts an optional final newline on non-`WRITE` requests; for binary `WRITE`, use a newline separator and pass exactly the required bytes.

**Implementation detail:** the receive callback stores at most one pending request. While a request is pending, additional datagrams may be silently dropped. There are no sequence numbers, duplicate detection, or delivery guarantees; a lost reply does not prove that a write did not occur.

## Allowed paths and operations

| Path | READ | WRITE | ENABLE / DISABLE |
|---|---|---|---|
| `/dev/fb` | 8 raw bytes | Exactly 8 raw bytes | Turn matrix scanning on / off |
| `/dev/gpio/<pin>` | ASCII `0\n` or `1\n` | ASCII `0` or `1` (optional trailing newline) | Set output / input direction |
| `/dev/pwm/1/freq` | Decimal ASCII with newline | Decimal frequency | Not supported |
| `/dev/pwm/1/a`, `/dev/pwm/1/b` | Decimal ASCII duty value | Decimal duty value | Not supported |
| `/dev/adc/0`, `/dev/adc/1`, `/dev/adc/2`, `/dev/adc/temp` | Formatted ASCII measurement | Not supported | Not supported |

GPIO access is restricted by `pin_gpio()` and the service's explicit safety check: **GPIO 23–28 are never remotely available**. Other pins are available only when not reserved for PWM, UART, ADC, or an active LED framebuffer. Valid pins depend on the firmware pin configuration. This is **not** a general remote VFS: paths such as `/etc/index.html` cannot be opened via UDP 2000. See [Remote GPIO control](REMOTE_GPIO.md) for pin-specific examples and [Device Guide](../devices/README.md) for local device details.

## Raw request example

On a Linux host with netcat (option behavior varies by version):

```sh
printf 'READ /dev/adc/temp' | nc -u -w 1 192.168.4.1 2000
printf 'WRITE /dev/pwm/1/freq\n50' | nc -u -w 1 192.168.4.1 2000
```

For robust parsing of replies (especially binary data), prefer `host/piconixctl.py`. The kernel implementation is in `kernel/net_service_remote_devctl.c`; requests use the existing device drivers, not task FDs or general file access. Physical effects and Wi-Fi availability must be verified on real hardware.

---

**Related links:** [Reference index](../README.md) | [Learning roadmap](../../ROADMAP.md) | [日本語](../../../ja/reference/network/REMOTE-DEVICE.md) | [Networking](NETWORK.md)
