# scroll: LED text ticker using select and UDP

## Purpose and usage

`scroll` is a user-space 8×8 LED ticker. It uses `select()` to multiplex nonblocking stdin and an optional UDP socket, with a 1024-byte ring buffer between input and 5×7 ASCII rendering. No new kernel service is required.

```sh
echo HELLO | scroll
scroll -u 2001 &
mcpd &
```

`-u` binds UDP port 2001 on all interfaces (Pico W). The LED framebuffer is `/dev/fb`; avoid concurrent GPIO 0–15 use. Without `-u`, only stdin is used. Canonical terminal input may require Enter.

## Data flow

```text
stdin ─────────────┐
                   ├─ select() ─→ 1024-byte ring ─→ 5×7 font ─→ /dev/fb
UDP port 2001 ─────┘
MCP client ─→ mcpd TCP 8080 ─→ UDP 127.0.0.1:2001
```

Printable ASCII 0x20–0x7e is supported. Control characters from stdin are ignored; UTF-8 Japanese is unsupported. Each glyph uses only its nonempty columns (leading and trailing blank columns are trimmed), with exactly one dark LED column between adjacent visible glyphs. The centers of lit pixels are therefore two LED pitches apart; omitting the dark column makes glyphs touch. A space character makes two dark columns in total for word separation. The framebuffer stores eight bytes, one per row.

## Plain-text UDP

UDP datagrams contain plain ASCII text with no request ID or header. Printable bytes (0x20–0x7e) are appended to the ring; control characters such as newlines are ignored. If the ring is full, excess characters are dropped. `scroll` sends no UDP reply.

```sh
scroll -u 1234 &
echo hello | nc -u 127.0.0.1 1234
```

The MCP `display_message` tool sends 1–128 ASCII bytes to localhost UDP 2001, without waiting for an ACK. A successful `sendto()` only means the local network stack accepted the bytes; it does not confirm delivery, queueing or display. In this educational implementation, escaped JSON characters and quotes within messages are unsupported. Start `scroll -u 2001 &` for MCP integration.

## Experiments

1. Run `echo HELLO | scroll` to examine pipe input and EOF.
2. Start `scroll -u 1234 &` and use `echo hello | nc -u 127.0.0.1 1234`.
3. Start `scroll -u 2001 &` and `mcpd &` to test `display_message`.
4. Inspect `select`, `read`, `recvfrom`, `sendto`, and `write` with `trace`.
5. Explore dropped bytes when the buffer fills and the limits of unreliable UDP delivery.

## Limitations and security

The initial implementation advances the display after a 100 ms `select()` timeout, or after a 100 ms sleep when input arrives. Frame timing is therefore approximate, not a precise periodic timer. Verify timing on hardware. The framebuffer remains enabled when the command exits. UDP is unauthenticated; use only a trusted experimental network.
