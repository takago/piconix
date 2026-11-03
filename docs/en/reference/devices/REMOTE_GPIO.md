# UDP remote GPIO control (experimental)

The kernel UDP service listens on port 2000. Its requests are not authenticated.
Only use it on a trusted isolated PicoNIX access point.

Examples from Linux:

```sh
printf 'READ /dev/gpio/16\n' | nc -u -w 2 192.168.4.1 2000
printf 'ENABLE /dev/gpio/22\n' | nc -u -w 2 192.168.4.1 2000
printf 'WRITE /dev/gpio/22 1\n' | nc -u -w 2 192.168.4.1 2000
printf 'WRITE /dev/gpio/22 0\n' | nc -u -w 2 192.168.4.1 2000
printf 'DISABLE /dev/gpio/22\n' | nc -u -w 2 192.168.4.1 2000
```

`ENABLE` switches to output; `DISABLE` switches to input. `READ` returns
`OK\n0\n` or `OK\n1\n`; successful writes return `OK\n`.
The service allows GPIO 0..17 and 22 if the pin configuration permits.
GPIO 0..15 are unavailable while the LED framebuffer is enabled.
GPIO 18/19 (PWM), 20/21 (UART), 23..25 (Pico W hardware), and 26..28
(ADC) are not remotely accessible. In particular, GP25 is not the Pico W
onboard LED (which is driven by CYW43).

The GPIO implementation delegates to existing `gpio_driver_rw` and
`gpio_driver_ioctl` to retain device access checks. The protocol remains
intentionally restricted; it does not expose arbitrary VFS paths.
