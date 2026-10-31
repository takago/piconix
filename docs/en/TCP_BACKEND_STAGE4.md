# TCP backend stage 4 (experimental)

This stage introduces a bounded TCP listening backend in `kernel/net_socket.c`.
It is **not yet connected to the PicoNIX socket system calls**. The existing
`SYS_SOCKET` still accepts UDP only. There is no TCP `nc` command yet.

Functions: `net_socket_tcp_listen`, `net_socket_tcp_connected`,
`net_socket_tcp_read`, `net_socket_tcp_write`, `net_socket_tcp_close`.
The backend accepts one connection, buffers at most 1024 received bytes, and
uses lwIP TCP flow control. It requires integration with the user FD lifecycle,
TCP accept/connect, blocking semantics, and clean reconnect handling before use.

The active TCP PCB limit was raised from 1 to 2 to allow Telnet and one
user TCP connection. HTTP is not implemented and will need further sizing.

Existing Python regression tests passed. No ARM firmware build or hardware
validation was performed. This is an internal development checkpoint only.
