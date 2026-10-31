# UDP Device Service (Pico W)

Pico W exposes selected device files over UDP port 2000. Connect to the PicoNIX Wi-Fi access point first.

## One-shot requests with netcat

```sh
printf 'READ /dev/adc/0' | nc -u -w 1 192.168.4.1 2000
printf 'WRITE /dev/pwm/1/freq\n50' | nc -u -w 1 192.168.4.1 2000
printf 'WRITE /dev/pwm/1/a\n7.5' | nc -u -w 1 192.168.4.1 2000
printf 'ENABLE /dev/gpio/fb' | nc -u -w 1 192.168.4.1 2000
printf 'WRITE /dev/gpio/fb\n\x18\x3c\x7e\xff\xff\x7e\x3c\x18' | nc -u -w 1 192.168.4.1 2000
printf 'DISABLE /dev/gpio/fb' | nc -u -w 1 192.168.4.1 2000
```

`WRITE` separates the path from its payload with a newline. Responses include an `OK\n` prefix, so use `host/piconixctl.py` or another parser for binary framebuffer reads. `nc` timeout behavior varies by implementation.

## Continuous Python demo

`host/demo_udp.py` animates a moving LED on the 8×8 framebuffer while repeatedly sampling ADC0. Press Ctrl+C to stop; it attempts to disable the framebuffer on exit.

```sh
python3 host/demo_udp.py --interval 0.2
```

`host/piconixctl.py` remains available for one-shot commands. Both use only the Python 3 standard library and do not retry requests.

One UDP request produces one reply. Requests are `READ <path>`, `WRITE <path>\n<bytes>`, `ENABLE <path>`, and `DISABLE <path>`. Replies are `OK\n` followed by raw bytes, or `ERR`. Maximum request size is 128 bytes. The framebuffer uses 8 binary bytes; PWM and ADC use ASCII numbers.

Allowed paths: `/dev/gpio/fb`, `/dev/pwm/1/freq`, `/dev/pwm/1/a`, `/dev/pwm/1/b`, `/dev/adc/0` through `/dev/adc/2`, and `/dev/adc/temp`. `ENABLE` and `DISABLE` apply only to the framebuffer.

There are no retries, request IDs, or authentication. Lost datagrams cause a client timeout. Use only on trusted local networks. Motor control requires a separate fail-safe for lost connectivity.

Locally, ADC sampling now occurs on the first `read()` after `open()`; subsequent reads drain the result and reach EOF. Reopen to sample again.
