# PicoNIX HTTP server (experimental)

`httpd` runs in user space on Pico W TCP port 80 and is launched by its startup script; it is not a kernel service.

`GET /` serves `/etc/index.html`. The browser page allows touching and dragging across the 8×8 LED matrix. PWM controls have been removed from the page, but the PWM HTTP API remains available through `curl`.

| API | Action | POST body |
|---|---|---|
| `GET /api/fb` | Read framebuffer as 16 hex digits | — |
| `POST /api/fb` | Write 8 bytes and enable matrix | `3C42A581A599423C` |
| `GET /api/pwm/freq` | Read PWM frequency (Hz) | — |
| `POST /api/pwm/freq` | Set PWM frequency | `1000` (8–100000) |
| `GET /api/pwm/a`, `/api/pwm/b` | Read duty cycle (%) | — |
| `POST /api/pwm/a`, `/api/pwm/b` | Set duty cycle | `50` or `50.0` (0–100) |

From a Linux machine, substitute the Pico W's hostname or IP address for `HOST`:

```sh
curl http://HOST/api/fb
curl -X POST --data-binary '3C42A581A599423C' http://HOST/api/fb
curl -X POST --data-binary '1000' http://HOST/api/pwm/freq
curl -X POST --data-binary '50' http://HOST/api/pwm/a
```

The page is stored in the volatile `/etc/index.html` RAM file and reverts on reboot. `/etc` and `/tmp` share eleven file slots of 2048 bytes each. Keep the HTML page at or below 2048 bytes.

**Limits and safety:** sequential, single-connection HTTP/1.0-style server. POST requires `Content-Length` and permits at most 32 body bytes. No chunked requests, HTTPS, authentication, concurrent requests, or keep-alive. Use only on an isolated, trusted laboratory network; never expose TCP 80 to the Internet. Do not directly connect hazardous actuators to PWM pins.

Telnet TCP 23 and the UDP 2000 device service remain kernel-side services.

### If a browser cannot open the page

Use `http://<Pico W IP address>/` explicitly; check that the browser does not upgrade it to HTTPS. Browsers may send larger request headers than `curl`. The server accepts request headers of up to 2048 bytes and replies with HTTP 413 if exceeded. Enable JavaScript to use the controls.

### Dragging across the LED matrix

Drag a finger over the matrix on a phone to paint or erase several LEDs in one gesture. The initial cell selects paint or erase mode, which remains fixed for the gesture. Mouse dragging works too. The browser display updates immediately. Pending framebuffer changes are sent at most once every 50 ms, with only one HTTP request in flight; changes made while sending are coalesced into the newest 8-byte frame. The final state remains pending after release. Clear and All use the same limit. Failed requests are not automatically retried.

### Periodic framebuffer synchronization

The browser polls `GET /api/fb` about every 250 ms without caching so changes made by another program or `curl` appear on the page. Polling is skipped while touching the matrix, while local edits are pending, or while an HTTP request is active. GET and POST never overlap. Edited frames are sent with the existing 50 ms minimum interval, subject to the Pico W response time.

`fbctl test` switches frames at 500 ms intervals; the former one-second poll could repeatedly sample the same phase.

The web page has `Clear` and `All` buttons. There is no manual `Read` button because the framebuffer is polled automatically every 250 ms.


### Web pages and PWM demo

- `/` (`/index.html`): small introduction and demo links.
- `/fbcheck.html`: LED matrix drawing (50 ms minimum write interval, 250 ms reads).
- `/pwm.html`: select servo on GP19/PWM1 B or passive buzzer on GP18/PWM1 A.

PWM output starts disabled. Start enables the selected mode; Stop sets both PWM duties to zero. Servo: 50 Hz, pulse width 0.5–1.5 ms in 0.02 ms steps (2.5–7.5% duty). Buzzer: fixed 50% duty, adjustable 100–5000 Hz. Mode changes stop both channels. Both channels share PWM slice 1 and cannot run at independent frequencies. Use a suitable external supply for the servo, common GND, and verify its permitted pulse-width range.

When a smartphone cannot reach the PicoNIX AP web page, try disabling mobile data/VPN temporarily and use `http://192.168.4.1/` directly.

The LED grid suppresses touch scrolling and text selection; gestures outside remain available.
