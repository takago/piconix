# Pico W Kernel Networking: Implementation and Extension

> **Document type: DEVELOPMENT GUIDE (not a textbook chapter or API reference).** [Network development index](README.md) | [Textbook C5](../../learn/C/C5.md) | [日本語](../../../ja/development/network/NETWORK-DEVELOPMENT.md)

This guide is a source-reading and modification map for **kernel-side** Wi-Fi, lwIP, sockets, and built-in services. The **user-space** `socket()` API is documented in [Network Programming in C](../apps/c/NETWORK-PROGRAMMING.md); current connection behavior and limits belong in the [Network Reference](../../reference/network/README.md). Study networking **after [device drivers](../drivers/DRIVER-DEVELOPMENT.md)** as an optional topic.

## 1. Responsibility map

| Component | Source | Focus |
|---|---|---|
| Startup and idle execution | [`kernel/main.c`](../../../../kernel/main.c) | `net_init()` and `net_poll()` in `idle_task()` |
| Wi-Fi, lwIP initialization and polling | [`kernel/net.c`](../../../../kernel/net.c), [`kernel/net.h`](../../../../kernel/net.h) | AP/STA, DHCP, mDNS, polling |
| User socket backend | [`kernel/net_socket.c`](../../../../kernel/net_socket.c), [`kernel/net_socket.h`](../../../../kernel/net_socket.h) | TCP/UDP state, send, receive, close |
| Syscalls and FDs | [`kernel/kernel.c`](../../../../kernel/kernel.c) | Allocation/release of socket FDs and syscall dispatch |
| Built-in remote shell | [`kernel/net_service_telnet.c`](../../../../kernel/net_service_telnet.c) | TCP 23, network TTY, disconnection |
| Built-in device control | [`kernel/net_service_remote_devctl.c`](../../../../kernel/net_service_remote_devctl.c) | UDP 2000 requests |
| Packet tracing and ICMP | [`kernel/net_capture.c`](../../../../kernel/net_capture.c), [`include/piconix_net_capture.h`](../../../../include/piconix_net_capture.h) | Capture records and echo requests/replies |
| lwIP options | [`kernel/lwipopts.h`](../../../../kernel/lwipopts.h) | Buffers, memory pools, TCP options |
| User-space service for comparison | [`userland/apps/httpd.c`](../../../../userland/apps/httpd.c) | HTTP is not a built-in kernel service |

`kernel/net_services.h` declares internal service entry points. Use `rg -n 'net_init\(|net_poll\(|net_socket_|net_service_' kernel/` to locate functions; source line numbers can change.

## 2. Reading order and two different paths

**First follow the shared flow:** `main()` → `net_init()` → `idle_task()` → `net_poll()`. On Pico W, the idle task provides execution opportunities for `cyw43_arch_poll()` and network services. `net_poll()` is not a timer interrupt, and a minimum poll interval does not guarantee a fixed schedule. See the [Network Reference](../../reference/network/NETWORK.md) for current diagnostics and settings.

Then trace these **separately**:

1. **User TCP/UDP sockets:** [`userland/apps/httpd.c`](../../../../userland/apps/httpd.c) and other applications → [`userland/lib/syscalls.c`](../../../../userland/lib/syscalls.c) → SVC/FD processing in `kernel/kernel.c` → `net_socket.c` → lwIP. The normal process lifecycle governs `httpd`.
2. **Kernel-resident services:** Telnet on TCP 23 and remote device control on UDP 2000, initialized by `net_init()` in AP mode or `sta_start_services()` after connection in STA mode → callbacks and polling in `net_service_*.c` → terminal or device handling as needed. Do not confuse their lifetime with user-process socket FDs.

`net_capture.c` provides separate packet observation and ICMP hooks. Packet records are **not** raw Ethernet frames.

## 3. Choose source files by the intended change

| Change | Read first | Verify afterward |
|---|---|---|
| AP/STA association, reconnection, DHCP, mDNS | `net.c` | Reconnect, Telnet lifetime, `/proc/net/wifi` |
| Polling order/interval | `net.c`, `main.c` | Idle scheduling, terminal responsiveness, `POLL_MAX_GAP_MS` |
| TCP/UDP I/O and socket lifetime | `net_socket.c`, `kernel.c` | EOF, shutdown, FD release, slot reuse |
| Telnet and network TTY | `net_service_telnet.c`, `kernel.c` | Independence from USB console, reconnect |
| UDP device control | `net_service_remote_devctl.c` | Allowed paths, input length, callback context |
| `ping` and `tcpdump` | `net_capture.c`, `include/piconix_net_capture.h` | Record format, bounded queues, dropped records |
| Public socket API changes | `kernel.c`, `net_socket.c`, `userland/lib/syscalls.c` | [Syscall reference](../../reference/SYSCALLS.md), ABI, user API |

## 4. Invariants to preserve

- **Polling context:** Avoid careless reentrancy into the lwIP raw API from callbacks. Check the existing deferred operations and polling flow, especially for UDP replies and loopback.
- **Buffer ownership:** Review TCP pbuf handling on `ERR_MEM`, `tcp_recved()`, and the send queue around `tcp_write()`. The [TCP behavior reference](../../reference/network/NETWORK.md) documents the current rules; changes can cause double-free, duplicated sends, or ACK inconsistencies.
- **FD and process lifetime:** Confirm that exiting tasks, `close()`, and terminated listeners release associated kernel socket slots.
- **RAM pressure:** TCP slots, pbuf pools, lwIP heap, and idle stack use RAM. Compare `/proc/net/memory` and the build's memory report before and after changes.
- **Board portability:** Preserve the `PICONIX_WIFI`-guarded stub implementations for ordinary Pico builds; verify both Pico W and Pico.
- **Security:** Telnet and UDP 2000 control do not provide authentication or encryption. Test only on an isolated trusted lab network.

## 5. Verification workflow

Record baseline behavior before modifying code. Use [Textbook B6](../../learn/B/B6.md) for an introductory hardware exercise.

```sh
# From the repository root on a development PC: source-level regression tests
python3 tools/run_tests.py

# Compile Pico W without flashing (requires Pico SDK and toolchain)
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
  ./tools/build_sdk.sh --no-flash -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel

# Check that the ordinary Pico build still works
PICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.1.1" \
  ./tools/build_sdk.sh --no-flash -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel
```

Keep a separate observation channel such as the USB console during on-device network experiments. On Pico W inspect:

```sh
cat /proc/net/wifi
cat /proc/net/memory
net
lsof
```

Test AP, STA if used, Telnet connect/disconnect, user sockets (`httpd`, `nc`), `127.0.0.1` loopback, and repeated connections. `tcpdump` and `ping` support observation. **Passing regression tests and compilation does not establish hardware network stability.**

## 6. Where to update documentation

- **User-visible behavior, connection methods and limits:** [Technical network reference](../../reference/network/README.md)
- **User C application API and compatibility rules:** [Network programming in C](../apps/c/NETWORK-PROGRAMMING.md)
- **Kernel implementation and modification guidance:** This guide
- **Sequential learning and exercises:** [Textbook C5](../../learn/C/C5.md)

If implementation and documentation diverge, inspect current source and update **both Japanese and English** pages.
