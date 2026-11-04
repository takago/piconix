# PicoNIX HTTP server (experimental)

*Technical reference*

`httpd` runs in user space on Pico W TCP port 80 and is launched by its startup script; it is not a kernel service.

`GET /` serves the introduction page `/etc/index.html`. The 8×8 LED matrix can be edited through `/fbcheck.html`, and the servo/buzzer demo is on `/pwm.html`. The APIs can also be exercised with `curl` on a host PC; PicoNIX itself does **not** include a `curl` command.

| API | Action | POST body |
|---|---|---|
| `GET /api/fb` | Read framebuffer as 16 hex digits | — |
| `POST /api/fb` | Write 8 bytes and enable matrix | `3C42A581A599423C` |
| `GET /api/pwm/freq` | Read PWM frequency (Hz) | — |
| `POST /api/pwm/freq` | Set PWM frequency | `1000` (8–100000) |
| `GET /api/pwm/a`, `/api/pwm/b` | Read duty cycle (%) | — |
| `POST /api/pwm/a`, `/api/pwm/b` | Set duty cycle | `50` or `50.0` (0–100) |
| `POST /api/tone` | Set PWM1 A buzzer frequency/duty together; `0` stops it | `0` or integer `100`–`5000` (Hz) |

From a Linux machine, substitute the Pico W's hostname or IP address for `HOST`:

```sh
curl http://HOST/api/fb
curl -X POST --data-binary '3C42A581A599423C' http://HOST/api/fb
curl -X POST --data-binary '1000' http://HOST/api/pwm/freq
curl -X POST --data-binary '50' http://HOST/api/pwm/a
```

The three Pico W pages (`/etc/index.html`, `/etc/fbcheck.html`, and `/etc/pwm.html`) are volatile RAM files restored on reboot. `/etc` and `/tmp` share eleven file slots of 2048 bytes each. Keep the HTML page at or below 2048 bytes.

**Limits and safety:** sequential, single-connection HTTP/1.0-style server. POST requires a valid, nonduplicate `Content-Length` and permits at most 32 body bytes. Malformed lengths are rejected, and a TCP EOF during request reception ends the wait immediately. No chunked requests, HTTPS, authentication, concurrent requests, or keep-alive. Use only on an isolated, trusted laboratory network; never expose TCP 80 to the Internet. Do not directly connect hazardous actuators to PWM pins.

Telnet TCP 23 and the UDP 2000 device service remain kernel-side services.

### If a browser cannot open the page

Use `http://<Pico W IP address>/` explicitly; check that the browser does not upgrade it to HTTPS. Browsers may send larger request headers than `curl`. The server accepts request headers of up to 2048 bytes and replies with HTTP 413 if exceeded. Enable JavaScript to use the controls.

### Dragging across the LED matrix

Drag a finger over the matrix on a phone to paint or erase several LEDs in one gesture. The initial cell selects paint or erase mode, which remains fixed for the gesture. Mouse dragging works too. The browser display updates immediately. Pending framebuffer changes are sent at most once every 50 ms, with only one HTTP request in flight; changes made while sending are coalesced into the newest 8-byte frame. The final state remains pending after release. Clear and All use the same limit. Failed requests are not automatically retried.

### Periodic framebuffer synchronization

The browser polls `GET /api/fb` about every 50 ms without caching so changes made by another program or `curl` appear on the page. Polling is skipped while touching the matrix, while local edits are pending, or while an HTTP request is active. GET and POST never overlap. Edited frames are sent with the existing 50 ms minimum interval, subject to the Pico W response time.

`fbctl test` switches frames at 500 ms intervals; the former one-second poll could repeatedly sample the same phase.

The web page has `Clear` and `All` buttons. There is no manual `Read` button because the framebuffer is polled automatically every 50 ms.


### Web pages and PWM demo

- `/` (`/index.html`): small introduction and demo links.
- `/fbcheck.html`: LED matrix drawing (50 ms minimum write interval, 50 ms reads).
- `/pwm.html`: select servo on GP19/PWM1 B or passive buzzer on GP18/PWM1 A.

PWM output starts disabled. Moving the servo slider automatically enables GP19 (50 Hz); the **Servo enabled** checkbox can also enable or stop it. Pressing a buzzer key automatically stops GP19 and activates GP18; releasing the key stops the buzzer. The two outputs share PWM slice 1 and cannot operate at independent frequencies. Servo pulse width is 0.7–2.3 ms in 0.02 ms steps (3.5–11.5% duty). Buzzer uses 50% duty at 262–523 Hz. Use an appropriate external servo supply and common GND.

When a smartphone cannot reach the PicoNIX AP web page, try disabling mobile data/VPN temporarily and use `http://192.168.4.1/` directly.

The LED grid suppresses touch scrolling and text selection; gestures outside remain available.

The browser serializes all servo and buzzer control requests because GP18 and GP19 share PWM slice 1. The servo requires 50 Hz; an outstanding buzzer request could otherwise restore an audio frequency after moving the servo slider. Check external servo power, ground sharing, and duty/frequency with `pwmctl status` if it still does not move.

To edit the introduction page on Pico W, run `edit /etc/index.html` and save with Ctrl+S. This modifies the RAM copy only; reboot restores the original content.

---

**Related links:** [Reference index](../README.md) | [Learning roadmap](../../ROADMAP.md) | [日本語](../../../ja/reference/network/HTTPD.md)
