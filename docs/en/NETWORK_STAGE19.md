# Stage 19: Remote device service refactoring

- Moved UDP 2000 command parsing and the restricted device allowlist from `kernel/kernel.c` to `kernel/net_service_remote_devctl.c`.
- Kept READ, WRITE, ENABLE and DISABLE protocol semantics and the deferred lwIP callback processing unchanged.
- Moved the shared PWM value parser to `kernel/pwm.c` and declared it in `kernel/devices.h`; local VFS and remote device control now share one parser.
- Updated source-structure regression checks.

The existing Telnet service (TCP 23), userland HTTP server (TCP 80), and socket interface are not modified.

Validation: Python regression suite passes. Full ARM firmware build and Pico W hardware validation are pending.
