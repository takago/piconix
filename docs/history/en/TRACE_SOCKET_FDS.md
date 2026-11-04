# Socket descriptors in lsof and trace (experimental)

> **Historical development note (not current specifications).** This file records an intermediate implementation stage. For current behavior, consult [Overview](../../en/guides/README.md), [Internals](../../en/reference/INTERNAL.md), [Pipes](../../en/guides/PIPE.md), and [Network programming](../../en/development/apps/c/NETWORK-PROGRAMMING.md).


`lsof` reads `/proc/task/<pid>`; socket descriptors now have readable names:
`socket:[udp]`, `socket:[tcp]`, and `socket:[tcp-peer]`.
These are descriptor types, not filesystem paths or network endpoints.

`trace` now labels syscall IDs 27..35, including socket, bind, sendto,
recvfrom, listen, connect, tcp_status and accept. These are internal
PicoNIX syscalls: BSD compatibility wrappers may map multiple calls onto
one syscall. The trace event format remains unchanged.

Limitations: local/remote IP addresses, port numbers, and TCP connection
states are not currently exposed in `/proc/task/<pid>`. Socket syscalls
are displayed by their internal names; `trace` does not decode sockaddr.
