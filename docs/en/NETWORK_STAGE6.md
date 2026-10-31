# Experimental TCP client backend (stage 6)

Adds `os_tcp_connect(fd, &endpoint)` and `os_tcp_status(fd)` to the experimental TCP socket API.
`os_tcp_connect` starts a nonblocking outbound TCP connection. `os_tcp_status` returns 1 when connected, 0 while connecting, -1 on error, or -2 after peer EOF. The `peer_ip` field follows the existing UDP packet representation.

Limitations: one TCP socket globally, existing listener still uses the same FD for accepted data, no separate `accept()` FD, no automatic blocking wait, no `nc` command. This stage is not a BSD-compatible socket implementation.

No Pico W hardware test or firmware build was performed in this environment.
