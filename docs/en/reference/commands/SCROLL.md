# scroll: LED text ticker using select and UDP

## Purpose and usage

`scroll` is a user-space 8×8 LED ticker. It uses `select()` to multiplex nonblocking stdin and an optional UDP socket, with a 1024-byte ring buffer between input and 5×7 ASCII rendering. No new kernel service is required.

```sh
echo HELLO | scroll
scroll -u 2001 &
```

`-u` binds UDP port 2001 on all interfaces (Pico W). The LED framebuffer is `/dev/fb`; avoid concurrent GPIO 0–15 use. Without `-u`, only stdin is used. Canonical terminal input may require Enter.

## Data flow

```text
stdin ─────────────┐
                   ├─ select() ─→ 1024-byte ring ─→ 5×7 font ─→ /dev/fb
UDP port 2001 ─────┘
```

Printable ASCII 0x20–0x7e is supported. Control characters from stdin are ignored; UTF-8 Japanese is unsupported. Each glyph uses only its nonempty columns (leading and trailing blank columns are trimmed), with exactly one dark LED column between adjacent visible glyphs. The centers of lit pixels are therefore two LED pitches apart; omitting the dark column makes glyphs touch. A space character makes two dark columns in total for word separation. The framebuffer stores eight bytes, one per row.

## Plain-text UDP

UDP datagrams contain plain ASCII text with no request ID or header. Printable bytes (0x20–0x7e) are appended to the ring; control characters such as newlines are ignored. If the ring is full, excess characters are dropped. `scroll` sends no UDP reply.

The following command example uses the explicit UDP port **1234**; the default for `scroll -u` remains **2001**.

```sh
scroll -u 1234 &
echo hello | nc -u 127.0.0.1 1234
```

## Input and buffering examples

`echo HELLO | scroll` supplies stdin; `scroll -u 1234 &` with `echo hello | nc -u 127.0.0.1 1234` supplies UDP. The `trace` command can show `select`, `read`, and `recvfrom` activity. UDP delivery is not guaranteed. When input exceeds display capacity, the 1,024-byte ring drops excess characters.

## Limitations and security

The initial implementation advances the display after a 100 ms `select()` timeout, or after a 100 ms sleep when input arrives. Frame timing is therefore approximate, not a precise periodic timer. Verify timing on hardware. The framebuffer remains enabled when the command exits. UDP is unauthenticated; use only a trusted experimental network.
