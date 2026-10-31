# PicoNIX HTTP server (experimental)

`httpd` is a user-space process, not a kernel service. It serves `GET /` over TCP 80 using the existing BSD-style socket wrappers. Other requests receive HTTP 404. Only one connection is processed at a time.

The default page is `/etc/index.html`, a writable volatile RAM file initialized at boot. Edit it with `edit /etc/index.html` or replace it with `echo ... > /etc/index.html`. Changes are visible on the next request and are lost after reboot.

`/etc` and `/tmp` share eight file slots, each holding at most 1024 bytes; the default index page uses one slot. No Flash persistence is provided. TCP 23 Telnet and UDP 2000 device control remain kernel services.

Known limits: no HTTP keepalive, no multiple simultaneous clients, no request body, no MIME detection, no security hardening. The underlying socket implementation remains experimental.
