# Experimental TCP accept (stage 7)

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/guides/README.md), [Internals](../../en/reference/INTERNAL.md), [Pipes](../../en/guides/PIPE.md), and [Network programming](../../en/development/apps/c/NETWORK-PROGRAMMING.md).


`os_tcp_accept(listener_fd)` returns a distinct accepted FD, or `OS_EAGAIN` while no connection is pending. The listener FD remains open and may accept another client after the accepted connection is closed. The single TCP backend permits only one active accepted client. TCP client connections continue to use `os_tcp_connect()`. This is not yet a complete BSD socket interface.

**Known limits:** single TCP socket owner process, single active TCP connection, no blocking accept, no userland `nc` yet. ARM firmware and Pico W runtime verification required.
