# Initial RAM Files and /etc/rc

*Technical reference*

Initial RAM files are ordinary files under `userland/fs/` on the build host. They are embedded in Flash during the build and copied into RAM files at boot; HTML and sample source code need not be embedded as C strings in the kernel.

## Host directory layout

```text
userland/fs/
├── etc_pico/
│   └── rc
├── etc_picow/
│   ├── fbcheck.html
│   ├── index.html
│   ├── pwm.html
│   └── rc
└── tmp_common/
    ├── fbwave.py
    ├── test.s
    └── test.py
```

Directory names encode the destination (`etc` or `tmp`) and board selection (`common`, `pico`, or `picow`). `tmp_pico/` and `tmp_picow/` may be added when needed. `common` applies to both boards; a board-specific file overrides a common file with the same destination path. For example, `etc_picow/rc` becomes `/etc/rc` on Pico W.

**These host-side folders select initial files; they do not create arbitrary directories in PicoNIX.** Only the six supported folder names are accepted. Unknown folders and nested subdirectories are build-generation errors.

## Limits and build

Up to 11 initial files may be selected per build, each no larger than 2048 bytes. Internal VFS paths (such as `etc/rc`) must fit within 31 bytes; destinations are limited to `/etc` and `/tmp`. `tools/generate_initial_fs.py` checks these limits during generation.

After changing or adding files, rebuild and flash the firmware for the target board. Changes made to RAM files on the running device are volatile: reboot restores the Flash-embedded initial content.

`/tmp/fbwave.py` demonstrates BH1750 light sensing (I²C address `0x23`) with the 8×8 LED matrix. If no sensor is connected or an I²C transfer fails, the wave still runs with a fixed 150 ms delay.

## /etc/rc and boot

PicoNIX does not run a separate `init` process. **The first shell (PID 1) executes `/etc/rc` once.** Telnet shells and nested shells do not run it again. The boot reader reuses the shell command execution path; it skips empty lines and lines beginning with `#`. Use simple commands and an optional trailing `&`; the boot reader does not interpret pipelines or redirections.

In the current minimal configuration, `etc_pico/rc` prints a greeting, while `etc_picow/rc` launches the HTTP server and prints the greeting. The Pico build initializes four files (`/etc/rc` plus three `/tmp` samples); Pico W initializes seven (`/etc/rc`, three HTML files and the three `/tmp` samples):

Pico:

```sh
echo "Welcome to PicoNIX!"
```

Pico W:

```sh
httpd &
echo "Welcome to PicoNIX!"
```

See also [Filesystem](FILESYSTEM.md), [HTTP server](network/HTTPD.md), and [Networking](network/NETWORK.md).

---

**Related links:** [Reference index](README.md) | [Learning roadmap](../ROADMAP.md) | [日本語](../../ja/reference/INITIAL-FILES.md)
