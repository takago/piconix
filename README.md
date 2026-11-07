# PicoNIX

**A tiny UNIX-like operating system with user-space TCP/IP networking for Raspberry Pi Pico and Pico W.**

PicoNIX is an experimental UNIX-like OS for RP2040 microcontrollers. It combines a shell, multitasking, a virtual file system, device files, and a compact TCP/IP socket interface for ordinary user programs. 

Unlike firmware that exposes only a built-in network service, PicoNIX lets user applications create TCP and UDP clients and servers through familiar socket APIs.

**Development and authorship.** The design principles and specifications of PicoNIX are determined by its creator based on their knowledge and experience. The source code and documentation are generated and refined using ChatGPT in accordance with these decisions. The creator reviews the results and carries out build and hardware verification as development proceeds. See the [development diary](docs/DEVELOPMENT_DIARY.md) ([日本語](docs/DEVELOPMENT_DIARY.ja.md)) for the development process.

**開発体制について．** PicoNIXの設計方針と仕様は，作成者の知識と経験に基づいて決定しています．ソースコードとドキュメントは，これらの方針に従ってChatGPTを用いて生成・改良したものです．開発の経緯は[開発日誌](docs/DEVELOPMENT_DIARY.ja.md)をご覧ください．

## Highlights

- **XIP kernel** — Kernel instructions execute directly from external Flash through the RP2040 XIP (Execute in Place) mechanism, leaving more SRAM available for runtime data and user processes.
- **Flash-resident PIC applications** — User programs are stored as position-independent code (PIC) images in Flash and copied into fixed RAM slots for execution.
- **Pico W Wi-Fi hardware** — On Pico W, PicoNIX uses the onboard Infineon CYW43439 Wi-Fi chip through the Raspberry Pi Pico SDK, with access-point (AP) and station (STA) modes.
- **Open-source TCP/IP networking** — On Pico W, PicoNIX uses lwIP, a lightweight open-source TCP/IP stack, and provides user applications with a compact BSD-style IPv4 socket API (`socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, `recv()`, `sendto()`, and `recvfrom()`).
- **LED text ticker** — `scroll` renders printable ASCII on the 8×8 LED matrix from stdin or UDP 2001, with a 1 KiB ring buffer, `select()` and plain-text UDP reception. `mcpd` can send display text using `display_message` (delivery is not confirmed). See [scroll tutorial](docs/en/guides/SCROLL.md) ([日本語](docs/ja/guides/SCROLL.md)).
- **Network applications** — `nc`, `httpd`, `tcp_echod`, `udp_echod`, and the experimental MCP server `mcpd` run as ordinary user programs. The HTTP server serves an editable `/etc/index.html`.
- **MCP and local LLM demo (Pico W)** — `mcpd` exposes RP2040 temperature, Pico W Wi-Fi information, and LED text output through Streamable HTTP/JSON-RPC on TCP port 8080. On November 7, 2026, the developer confirmed on real hardware that an LLM could obtain the IP address with `get_wifi_info` and display it on the 8×8 LED matrix using `display_message` and `scroll`. See the [MCP guide](docs/en/guides/MCP.md) ([日本語](docs/ja/guides/MCP.md)).
- **Concurrent UDP clients and servers** — Up to three user UDP sockets with independent receive buffers enable local communication between `scroll` and `nc` (Pico W; the developer confirmed the integrated MCP-to-LED demonstration on real hardware on November 7, 2026).
- **Built-in network services** — A Telnet shell on TCP port 23 and remote device control on UDP port 2000 are implemented as separate kernel services.
- **UNIX-like environment** — A shell and familiar commands including `ls`, `cat`, `cp`, `grep`, `ps`, `kill`, `find`, and `tree`, with `/bin`, `/dev`, `/proc`, `/tmp`, and `/etc`. Binary transfer with `dd` is also supported.
- **Foreground interruption** — Ctrl+C terminates registered foreground tasks, including pipeline stages; `nc -u -N` exits on stdin EOF for one-shot UDP sends.
- **Six application slots** — Six fixed 24 KiB RAM slots (4 KiB stack reserve per slot) on RP2040.
- **Pipes and redirection** — Two- and three-command pipelines using `|`, with `pipe()`, `dup()`, and `dup2()` support.
- **Device files** — Access GPIO, ADC, PWM, I²C1, UART, and an LED-matrix framebuffer through file-like interfaces. Supported devices can also be controlled remotely over UDP.
- **System call interface** — A compact SVC-based ABI exposes process control, file and device I/O, pipes, and TCP/UDP networking. See the [system call reference](docs/en/reference/SYSCALLS.md) ([日本語](docs/ja/reference/SYSCALLS.md)).
- **Programming on the device** — Editors (`edit`, `vi`), an interactive file manager (`fd`), a binary-copy utility (`dd`), an assembler (`as`), and a small Python-like interpreter (`python`).
- **Inspect memory live (`mem`)** — Explore RAM contents and task memory while PicoNIX is running. See the [memory inspection guide](docs/en/guides/MEM.md) ([日本語](docs/ja/guides/MEM.md)).
- **Follow system activity (`trace`)** — Observe system calls, return values, I/O previews, and task switches to understand how user programs interact with the kernel. See the [trace guide](docs/en/guides/TRACE.md) ([日本語](docs/ja/guides/TRACE.md)).
- **Inspect open descriptors (`lsof`)** — See which files, devices, and sockets are held open by processes, including socket-port information.
- **Inspect network packets (`tcpdump`, Pico W)** — Capture and examine network traffic directly on the device, alongside `ip`, `ping`, and the user-space networking tools. See the [tcpdump reference](docs/en/reference/network/TCPDUMP.md) ([日本語](docs/ja/reference/network/TCPDUMP.md)).
- **Kernel function tracing (enabled in standard builds)** — Inspect selected kernel function entry/return events with `trace run hello`, or use `trace run -s hello` to record only system calls. Event capture starts OFF; the build option `PICONIX_KERNEL_FUNCTION_TRACE` defaults to ON. See the [trace guide](docs/en/guides/TRACE.md) ([日本語](docs/ja/guides/TRACE.md)) and [setup guide](docs/en/setup/SETUP.md) ([日本語](docs/ja/setup/SETUP.md)).
- **Terminal games** — Moon Buggy (`mb`) and `pong`.

## LLM-to-LED demo (Pico W)

The developer confirmed an end-to-end hardware demonstration on **November 7, 2026**: a local LLM called the MCP tool `get_wifi_info` to obtain the Pico W IP address, then used `display_message` to scroll that address across the 8×8 LED matrix.

Start the LED ticker from the PicoNIX shell:

```sh
scroll -u 2001 &
```

The data path is **LLM → MCP client → `mcpd` (TCP 8080) → `display_message` → UDP `127.0.0.1:2001` → `scroll` → LED matrix**. `mcpd` sends plain text via UDP and does not wait for an acknowledgement; a successful send alone does not guarantee that text has been displayed. `scroll` may also receive text through standard input or a plain-text UDP sender.

This example demonstrates composition of two MCP tools, user-space networking, `select()`-based I/O multiplexing, and physical device output. See the [MCP guide](docs/en/guides/MCP.md) ([日本語](docs/ja/guides/MCP.md)) and [scroll guide](docs/en/guides/SCROLL.md) ([日本語](docs/ja/guides/SCROLL.md)).

## Commands

PicoNIX includes familiar UNIX-style utilities as well as tools for inspecting the OS, programming the RP2040, and experimenting with networking. Run `help` for the command list or `help NAME` for usage. For example, `cat -n /tmp/test.py` displays line numbers.

| Category | Commands |
|---|---|
| Shell & files | `sh`, `ls`, `cat`, `cp`, `grep`, `pwd`, `echo`, `rm`, `find`, `tree`, `fd`, `dd` |
| Processes & diagnostics | `ps`, `kill`, `lsof`, `trace`, `mem`, `hexdump`, `watch` |
| Terminals | `getty`, `tty`, `stty`, `reset` |
| Programming | `edit`, `vi`, `as`, `cc`, `python` |
| Hardware | `gpio`, `pins`, `fbctl`, `fbed`, `pwmctl`, `i2cctl`, `bh1750`, `blink`, `scroll` |
| Networking | `ip`, `net`, `nc`, `ping`, `tcpdump`, `httpd`, `tcp_echod`, `udp_echod`, `mcpd` |
| Games & demos | `mb`, `pong`, `omikuji`, `hello`, `kitesys` |
| Help | `help` |

Shell features include redirection (`<`, `>`) and pipelines (`|`); these are shell operators rather than separate commands. On Pico W, network applications use the user-space socket API. See the [programming guide](docs/en/development/apps/c/PROGRAMMING.md) and [日本語の開発ガイド](docs/ja/development/apps/c/PROGRAMMING.md) for more details.

## Explore the running OS

PicoNIX is designed not only to run programs but also to let you inspect what happens inside the OS. Try these commands on the device:

```sh
mem                # Inspect RAM and task memory
lsof               # List open file descriptors
trace run -s hello # Observe only the system calls made by hello
```

### Trace system calls

`trace run -s` records system calls without KERNEL, TIMER or SWITCH events. The following excerpt is from an actual PicoNIX run:

```text
piconix# trace run -s hello
Hello, PicoNIX!

--- trace: hello (PID 14) ---
Type     Function    Arg              Ret Info
----------------------------------------------------
SYSCALL  write       1:/dev/console   --  --
SYSCALL  write/      1:/dev/console   16  "Hello, P"...
SYSCALL  exit        0                --  --
SYSCALL  exit/       0                0   --
--- end of trace ---
```

The trace shows the `write` system call, its return value and an output preview, followed by process exit. Time, PID, Command, and TTY columns are omitted from this excerpt to keep it readable.

### Trace selected kernel function calls

Kernel function instrumentation is **ON by default at build time**; event capture remains **OFF on boot** until `trace on` or `trace run`. Use `trace run -s hello` for syscall entry/exit only, `trace run hello` for syscall plus selected SVC kernel functions, and `trace run -a hello` for all-context selected functions. Explicitly build with `-DPICONIX_KERNEL_FUNCTION_TRACE=OFF` for a smaller firmware without `KERNEL` function events. The same `hello` program then reveals the nested calls inside `write`:

```text
piconix# trace run hello
Hello, PicoNIX!

--- trace: hello (PID 16) ---
Type     Function             Arg              Ret Info
-----------------------------------------------------------
SYSCALL  write                1:/dev/console   --  --
KERNEL     vfs_rw             --               --  --
KERNEL       in_task_range    --               --  --
KERNEL       in_task_range/   --               --  --
KERNEL       proc_device      --               --  --
KERNEL       proc_device/     --               --  --
KERNEL       device_rw_tty    --               --  --
KERNEL       device_rw_tty/   --               --  --
KERNEL     vfs_rw/            --               --  --
KERNEL     in_task_range      --               --  --
KERNEL     in_task_range/     --               --  --
SYSCALL  write/               1:/dev/console   16  "Hello, P"...
... process-exit events omitted ...
--- end of trace ---
```

These excerpts reproduce observed call names, order, and return values, but omit the repeated time/process columns and, in the kernel example, repetitive `fd_release` events on exit. A trailing `/` indicates a function return; indentation indicates call nesting. `--` means that an argument, return value, or supplemental information is not recorded or is not applicable. The former `trace kernel` command and its `-d` depth option are no longer supported. `trace run` displays selected function events when instrumentation is enabled.

See the [trace guide](docs/en/guides/TRACE.md) ([日本語](docs/ja/guides/TRACE.md)) for details, and the [setup guide](docs/en/setup/SETUP.md) ([日本語](docs/ja/setup/SETUP.md)) to enable experimental kernel tracing. On **Pico W**, `tcpdump` can also inspect network traffic; run `help tcpdump` for supported options.

## Network demo

Connect a Linux PC to the Pico W access point:

- SSID: `PicoNIX-XXXXXXXX` (device-specific suffix)
- Password: `piconix1234`
- Pico W address: `192.168.4.1`
- Linux PC address: `192.168.4.2` (single-client DHCP configuration; verify the assigned address)

These addresses apply to AP mode. In STA mode, use the addresses assigned by the existing network.

Open the PicoNIX shell from the PC:

```sh
telnet 192.168.4.1
```

The Pico W `/etc/rc` starts `httpd` automatically. Visit `http://192.168.4.1/` in a browser; `/` is the introduction page, `/fbcheck.html` controls the LED matrix, and `/pwm.html` controls PWM. To change the introduction page, run `edit /etc/index.html` (or `vi /etc/index.html`) and save. The editors open RAM files directly under `/tmp` and `/etc`; changes are lost on reboot. If `httpd` has been stopped, restart it from the shell:

```sh
# PicoNIX shell
httpd
```

Alternatively, start a TCP echo server and connect from Linux:

```sh
# PicoNIX shell
tcp_echod 1234
```

```sh
# Linux PC
nc 192.168.4.1 1234
```

For UDP, start `udp_echod 1234` on PicoNIX and run:

```sh
printf 'hello\n' | nc -u -w 2 192.168.4.1 1234
```

To send a one-shot UDP message in the other direction, run `nc -u -l 12345` on Linux and `echo hello | nc -u -N 192.168.4.2 12345` on PicoNIX.

### MCP server and LLM demo (Pico W)

PicoNIX can run `mcpd` as an **experimental network programming example in user space**. It does not require an MCP-specific kernel service and does not replace `httpd` on TCP port 80. Start it from the PicoNIX shell:

```sh
mcpd &
```

The server listens on **TCP 8080** at `POST /mcp` and implements a limited subset of MCP **2026-07-28** over Streamable HTTP (JSON responses; no SSE). Its two read-only tools are:

| Tool | Data source | Purpose |
|---|---|---|
| `get_temperature` | `/dev/adc/temp` | Read RP2040 internal temperature |
| `get_wifi_info` | `/proc/net/wifi` | Read Pico W Wi-Fi and IP configuration |

On a Linux PC connected to the Pico W AP, use the official MCP Python SDK v2 to list and call tools **without an LLM**:

```sh
python3 -m venv .venv
. .venv/bin/activate
pip install "mcp>=2,<3"
python host/mcp_client.py http://192.168.4.1:8080/mcp
```

For **local LLM integration**, install `openai` and configure the LLM URL, model and API key placeholders in `host/mcp_llm_client.py` before running it:

```sh
pip install openai
python host/mcp_llm_client.py
```

On **November 6, 2026**, hardware testing verified tool discovery, temperature/Wi-Fi readings, and natural-language tool selection by a local LLM using the official MCP Python SDK v2. For example, a question about PicoNIX status triggered both read-only tools. This is a proof of basic interoperability, **not** validation of complete MCP compliance or long-term reliability.

See the [MCP usage guide](docs/en/guides/MCP.md) ([日本語](docs/ja/guides/MCP.md)), [LLM client tutorial](docs/en/guides/MCP-LLM-CLIENT.md) ([日本語](docs/ja/guides/MCP-LLM-CLIENT.md)), and [MCP server development lab](docs/en/development/apps/c/MCP-SERVER-DEVELOPMENT.md) ([日本語](docs/ja/development/apps/c/MCP-SERVER-DEVELOPMENT.md)).

**MCP security:** `mcpd` has no authentication or TLS. Run it only on an isolated, trusted experimental network; do not expose TCP 8080 to the Internet.

**Security:** The built-in Telnet and remote device-control services do not provide authentication or encryption. Use them only on a trusted experimental network.

## Build

```sh
# Raspberry Pi Pico (without Wi-Fi)
./tools/build_sdk.sh -DPICO_BOARD=pico -DCMAKE_BUILD_TYPE=MinSizeRel

# Raspberry Pi Pico W (with Wi-Fi)
./tools/build_sdk.sh -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=MinSizeRel
```

The Pico SDK and an ARM toolchain are required. See the setup guide linked below for installation details.

## Architecture

### Execution and memory model

- **Kernel code:** Executes directly from external Flash using XIP.
- **User programs:** Stored in Flash as PIC images; the selected image is copied into a RAM application slot when launched and executes from SRAM.
- **SRAM:** Holds kernel data and stacks, user-process code/data/stacks, and other runtime state. PicoNIX uses six fixed 24 KiB application slots on RP2040.

This approach saves scarce SRAM by keeping kernel instructions in Flash while allowing user applications to run from separate RAM slots. See [Architecture](docs/en/reference/ARCHITECTURE.md) and [全体構造](docs/ja/reference/ARCHITECTURE.md) for details.

| Component | Responsibility |
|---|---|
| `kernel/kernel.c` | Tasks, system calls, and VFS |
| `kernel/net.c` | Wi-Fi and lwIP initialization and management |
| `kernel/net_socket.c` | Socket backend for user processes |
| `kernel/net_service_telnet.c` | Built-in Telnet service (TCP 23) |
| `kernel/net_capture.c` | IPv4 packet summaries and ICMP Echo for Pico W |
| `kernel/net_service_remote_devctl.c` | Remote device-control service (UDP 2000) |
| `userland/apps/nc.c`, `userland/apps/httpd.c` | User-space networking applications |
| `userland/apps/mcpd.c` | Experimental user-space MCP server (HTTP, JSON-RPC, device-file access) |
| `userland/apps/tcp_echod.c`, `userland/apps/udp_echod.c` | User-space echo servers |

The socket API is intentionally small rather than fully POSIX-compatible. `send()` and `recv()` currently support `flags = 0` only.

The volatile `/tmp` and `/etc` directories share a pool of up to eleven RAM files, each limited to 2 KiB. A separate 2 KiB region is exposed as `/dev/shm`; on Pico W, lwIP has a 10 KiB heap inside the 16 KiB network reservation and supports loopback to `127.0.0.1`. Use `cat /proc/net/memory` to inspect network allocation statistics. PicoNIX provides `ping` and `tcpdump` on Pico W, but does not provide a `curl` command (the examples in the HTTP guide run `curl` on a host PC). Initial files come from `userland/fs/` at build time; the PID 1 shell runs `/etc/rc` once on boot.

## Documentation

Choose the documentation that matches your purpose. Start with **Setup** if you are new to PicoNIX. The technical references below link directly to the most frequently consulted specifications.

| Category | English | 日本語 |
|---|---|---|
| **Setup** — Install the development tools | [Setup guide](docs/en/setup/SETUP.md) | [セットアップ手順](docs/ja/setup/SETUP.md) |
| **Learning roadmap** — Suggested order for learning UNIX | [Learning roadmap](docs/en/ROADMAP.md) | [学習ロードマップ](docs/ja/ROADMAP.md) |
| **Task index** — Find instructions by goal | [Task index](docs/en/TASKS.md) | [目的別索引](docs/ja/TASKS.md) |
| **Usage guides** — Commands and practical examples | [Usage guides](docs/en/guides/README.md) | [操作ガイド](docs/ja/guides/README.md) |
| **System specifications** — Features and limitations | [Specifications](docs/en/reference/SPECIFICATION.md) | [システム仕様](docs/ja/reference/SPECIFICATION.md) |
| **Architecture** — Kernel and process design | [Architecture](docs/en/reference/ARCHITECTURE.md) | [システム構成](docs/ja/reference/ARCHITECTURE.md) |
| **System calls** — ABI and call definitions | [System calls](docs/en/reference/SYSCALLS.md) | [システムコール一覧](docs/ja/reference/SYSCALLS.md) |
| **C library** — User-space C interfaces | [C library](docs/en/development/apps/c/LIBC.md) | [Cライブラリ](docs/ja/development/apps/c/LIBC.md) |
| **Memory layout** — Flash and RAM allocation | [Memory layout](docs/en/reference/MEMORY-LAYOUT.md) | [メモリ配置](docs/ja/reference/MEMORY-LAYOUT.md) |
| **Hardware** — Boards, pins and wiring | [Hardware](docs/en/reference/hardware/README.md) | [ハードウェア](docs/ja/reference/hardware/README.md) |
| **Devices** — Device files and interfaces | [Devices](docs/en/reference/devices/README.md) | [デバイス仕様](docs/ja/reference/devices/README.md) |
| **Networking** — Wi-Fi, TCP/IP and services | [Networking](docs/en/reference/network/README.md) | [ネットワーク仕様](docs/ja/reference/network/README.md) |
| **LED text ticker** — LED text and UDP/MCP integration | [Scroll guide](docs/en/guides/SCROLL.md) | [LEDスクロール](docs/ja/guides/SCROLL.md) |
| **C subset compiler** — On-device C compilation | [Compiler guide](docs/en/development/apps/c/CC.md) | [Cコンパイラガイド](docs/ja/development/apps/c/CC.md) |
| **MCP demo** — MCP server and Linux client | [MCP guide](docs/en/guides/MCP.md) | [MCP操作ガイド](docs/ja/guides/MCP.md) |
| **LLM integration** — Call MCP tools from a local LLM | [LLM client tutorial](docs/en/guides/MCP-LLM-CLIENT.md) | [LLM連携ガイド](docs/ja/guides/MCP-LLM-CLIENT.md) |
| **Development guides** — Applications and kernel extensions | [Development guides](docs/en/development/README.md) | [開発ガイド](docs/ja/development/README.md) |
| **Function index** — Browse source functions (Japanese only) | [Function index (Japanese)](docs/FUNCTIONS.md) | [関数索引](docs/FUNCTIONS.md) |
| **Development diary** — Project development history | [Development diary](docs/DEVELOPMENT_DIARY.md) | [開発日誌](docs/DEVELOPMENT_DIARY.ja.md) |

The learning roadmap gives the recommended sequence; usage guides and development guides provide the hands-on instructions. The reference pages describe the interfaces and constraints.

### On-device C subset compiler

`cc` translates a small educational C subset into Thumb-1 assembly for the built-in `as`. Try `cc /tmp/hello.c`, `as /tmp/cc.s`, then `./a.out`. Supported statements include `write(fd, "text", count);`, `getpid();`, `return N;`, integer locals, recursive-descent arithmetic expressions (`+`, `-`, `*`, `/`, `%`, parentheses), `if` and `while` inside `int main(void)`. This is **not** a general-purpose C compiler. [Japanese guide](docs/ja/development/apps/c/CC.md) / [English guide](docs/en/development/apps/c/CC.md).


### C subset compiler: characters and arrays

The educational `cc` now supports 32-bit `int`, byte-sized `char`, local fixed-size `int[]` and `char[]`, character constants, indexed reads/writes, `sizeof(local)`, and `char[]` string initialization. `read(fd, array, count_expression)` and `write(fd, array, count_expression)` now accept integer expressions for the file descriptor and byte count. All local storage uses a 256-byte stack frame. Array bounds are **not** checked at runtime; array-buffer arguments must be named local arrays (not general pointers). See the [Japanese guide](docs/ja/development/apps/c/CC.md) and [English guide](docs/en/development/apps/c/CC.md). This is not a conforming C compiler; native RP2040 execution is not yet verified.

### `cc` 回帰テスト / Compiler regression tests

`python3 tools/test_scripts/test_cc_regressions.py` checks accepted/rejected C subsets and Thumb assembly output. `python3 tools/test_scripts/test_cc_stack.py` uses the current 256-byte frame. These are host-side checks, **not** RP2040 execution tests.

> `cc` の比較演算は符号付きThumb-1条件分岐に対応しました．`tools/test_scripts/test_cc_signed_comparisons.py` は命令符号を検証しますが，実機実行テストではありません．

### Experimental C function definitions

The educational `cc` now emits Thumb-1 calls for user-defined `int`/`void` functions with up to four `int` parameters, per-call stack frames, nested calls, and recursion. See [日本語](docs/ja/development/apps/c/CC.md) / [English](docs/en/development/apps/c/CC.md). Assembly tests pass; RP2040 execution and full AAPCS stack alignment remain unverified.
