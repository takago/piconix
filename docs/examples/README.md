# PicoNIX documentation exercise files

These files support the Japanese and English hands-on documentation. They are **examples**, not automatically built into stock PicoNIX firmware.

| File | Usage |
|---|---|
| `udp_once.c` | Pico W UDP echo client. Copy to `userland/apps/`, register and flash |
| `tcp_get.c` | Pico W TCP HTTP client. Copy to `userland/apps/`, register and flash |
| `udp_echo_host.py` | Linux PC UDP echo server, used with `udp_once` |
| `uptime_lab.patch` | Optional source patch adding `getuptime()`, `/dev/uptime` and `labuptime` |
| `test_uptime_device.c` | Host-side test of the optional virtual driver after applying the patch |

Start with the [Japanese networking tutorial](../ja/development/apps/c/NETWORK-PROGRAMMING.md) or [English networking tutorial](../en/development/apps/c/NETWORK-PROGRAMMING.md). For kernel changes, read the [Japanese syscall tutorial](../ja/development/syscalls/SYSCALL-DEVELOPMENT.md), [Japanese driver tutorial](../ja/development/drivers/DRIVER-DEVELOPMENT.md), or the corresponding English documents.

The patch targets the specific repository version used for the exercises. Apply it only on a separate branch and first run `git apply --check docs/examples/uptime_lab.patch`. It does not modify the stock kernel merely by being present in the docs tree. Hardware tests and SDK builds must be done on your machine; the examples in the guides are not presented as captured logs.

After patch application, test the driver module on the Linux PC without Pico SDK:

```sh
cc -std=c11 -Wall -Wextra -Werror -Ikernel -Iinclude \
  docs/examples/test_uptime_device.c kernel/uptime_device.c -o /tmp/test_uptime_device
/tmp/test_uptime_device
```
