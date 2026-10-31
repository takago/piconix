# Experimental TCP accept (stage 7)

`os_tcp_accept(listener_fd)` returns a distinct accepted FD, or `OS_EAGAIN` while no connection is pending. The listener FD remains open and may accept another client after the accepted connection is closed. The single TCP backend permits only one active accepted client. TCP client connections continue to use `os_tcp_connect()`. This is not yet a complete BSD socket interface.

**Known limits:** single TCP socket owner process, single active TCP connection, no blocking accept, no userland `nc` yet. ARM firmware and Pico W runtime verification required.
