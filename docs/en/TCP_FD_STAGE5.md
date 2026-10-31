# Stage 5: experimental TCP FD integration

This stage extends the existing stage-4 TCP backend with an experimental
**server-side** socket descriptor. It is not a complete BSD socket API.

- `os_socket(OS_AF_INET, OS_SOCK_STREAM, 0)` allocates the single TCP socket FD.
- `os_tcp_listen(fd, 5000)` starts a TCP listener on the specified port.
- `os_read(fd, ...)` and `os_write(fd, ...)` transfer raw TCP bytes after a client connects.
- `os_close(fd)` or process exit closes the listener and client.
- `os_fcntl(fd, 2, OS_O_NONBLOCK)` is accepted. TCP I/O currently returns
  `OS_EAGAIN` when no data/connection is available, even without this flag.
- Only one user TCP socket owner and one incoming client are supported.

Limitations: no `accept()`, `connect()`, blocking TCP wait, or `nc` command yet.
An accepted connection is associated directly with the listener FD, so this is
not POSIX-compatible socket behavior. UDP APIs remain experimental. No ARM
firmware build or hardware validation was performed for this stage.

Next steps: implement proper accepted-socket descriptors and client connect,
then complete user-facing API wrappers and `nc`.
