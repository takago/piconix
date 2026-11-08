# PicoNIX Architecture

*Technical reference*

This guide maps the **current source tree** to major OS responsibilities. See the [specification](SPECIFICATION.md) for ABI and resource limits and the individual guides for detailed behavior.

## 1. Execution model

```text
                 User space (six 24 KiB slots)
             sh | grep | mem | python | httpd | ...
                        | syscall (SVC)
                        v
+---------------------------------------------------------+
| Kernel                                                  |
| Tasks / SysTick / PendSV / FDs / pipes / VFS / TTY       |
| Socket management / Telnet / UDP device control         |
+---------------------------------------------------------+
         |                 |               |
      USB / UART       GPIO, etc.     lwIP / Wi-Fi (Pico W)
         |                 |               |
                    RP2040 / Pico SDK
```

Registered user programs are stored as PIC images in Flash and copied into RAM slots when launched. Kernel instructions execute via XIP. There is no MMU, virtual memory, or per-process memory protection.

## 2. Source map

| Source | Responsibility |
|---|---|
| `kernel/main.c` | Startup and embedded app registration |
| `kernel/kernel.c` | Tasks, FDs, pipes, VFS, TTY, SVC handling |
| `kernel/context.S` | Context switching |
| `kernel/gpio.c`, `fb.c`, `adc.c`, `pwm.c` | Device drivers |
| `kernel/net.c`, `net_socket.c` | Network and socket infrastructure |
| `kernel/net_service_telnet.c`, `net_service_remote_devctl.c` | Kernel network services |
| `userland/apps/shell.c` | Parsing, redirection, pipelines, background tasks |
| `userland/lib/syscalls.c`, `crt0.c` | User API and startup |
| `userland/apps/httpd.c`, `nc.c` | User-space network tools |
| `include/piconix.h` | Public API and syscall-related definitions |
| `CMakeLists.txt`, `tools/build_sdk.sh` | Build, embedded images, post-link checks |

## 3. Boundaries

- `/bin` exposes embedded apps, `/tmp` and `/etc` are simple RAM-file directories, `/dev` provides devices, and `/proc` exposes runtime state and symbol data. See [Filesystem](FILESYSTEM.md).
- Standard FDs are normally 0, 1, and 2. Files, devices, pipes, and sockets use descriptor-based interfaces, without full POSIX compatibility. See [Pipes](commands/PIPE.md) and [System calls](SYSCALLS.md).
- The kernel manages TTY input settings and ONLCR; applications and host terminal emulators manage cursors, alternate screen, and display effects. See [TTY](commands/TTY.md) and [Terminal](commands/TERMINAL.md).
- On Pico W, Telnet (TCP 23) and UDP device control (UDP 2000) are kernel services; HTTP (TCP 80) is provided by user-space `httpd`. Availability depends on build configuration. See [Networking](network/NETWORK.md).

## 4. Suggested reading order

Start with `kernel/main.c`, `userland/apps/shell.c`, `userland/lib/syscalls.c`, `kernel/kernel.c`, and `kernel/context.S`. Follow network or device subsystems as needed. Update both language versions of the documentation and `userland/apps/help.c` whenever user-facing behavior changes.

---

**Related links:** [Reference index](README.md) | [Learning roadmap](../ROADMAP.md) | [日本語](../../ja/reference/ARCHITECTURE.md) | [Internals](INTERNAL.md) | [Documentation](../../DOCS.md)
