# Command reference

This index covers **all 54 applications** embedded by `CMakeLists.txt`, plus shell built-ins. Commands sharing names with UNIX utilities may have fewer options.

Syntax is abbreviated. Run `help NAME` for firmware help. Follow “Details” links for advanced tools. Some networking commands require Pico W.

**Implementation sources:** [`CMakeLists.txt`](../../../../CMakeLists.txt) (`foreach(APP IN ITEMS ...)`), [`help.c`](../../../../userland/apps/help.c), and [`shell.c`](../../../../userland/apps/shell.c).

## Files and shell

| Command | Purpose | Syntax / key options | Details |
|---|---|---|---|
| [`sh`](../../../../userland/apps/shell.c) | Interactive shell (command: sh) | `sh` | [Details](PIPE.md) |
| [`ls`](../../../../userland/apps/ls.c) | List directory contents | `ls [-la] [path]— -l: details, -a: . and ..` | — |
| [`tree`](../../../../userland/apps/tree.c) | Show directory tree | `tree [-L 0..8] [path]` | — |
| [`find`](../../../../userland/apps/find.c) | Find directory entries | `find [path] [-name pattern]` | — |
| [`fd`](../../../../userland/apps/fd.c) | File browser | `fd [directory]` | [Details](FD.md) |
| [`pwd`](../../../../userland/apps/pwd.c) | Print working directory | `pwd` | — |
| [`cat`](../../../../userland/apps/cat.c) | Show file or standard input | `cat [-n] [file]— -n: number lines` | — |
| [`cp`](../../../../userland/apps/cp.c) | Copy RAM file | `cp SRC DST` | — |
| [`rm`](../../../../userland/apps/rm.c) | Remove RAM file | `rm path` | — |
| [`echo`](../../../../userland/apps/echo.c) | Print text | `echo [-n] [-e] [text...]— -n: no newline, -e: escapes` | — |
| [`grep`](../../../../userland/apps/grep.c) | Filter lines by literal substring | `grep PATTERN [file]` | — |
| [`cut`](../../../../userland/apps/cut.c) | Extract selected delimiter-separated fields | `cut [-d CHAR] -f LIST [FILE]` | [Details](CUT.md) |
| [`hexdump`](../../../../userland/apps/hexdump.c) | Hex and ASCII dump | `hexdump [file [offset [count]]]` | — |
| [`dd`](../../../../userland/apps/dd.c) | Copy byte streams or RAM data | `dd [if=FILE] [of=FILE] [bs=1..256] [count=N] [skip=N] [seek=N]` | [Details](DD.md) |
| [`edit`](../../../../userland/apps/edit.c) | Text editor | `edit /tmp/FILE \| edit /etc/FILE` | [Details](EDIT.md) |
| [`vi`](../../../../userland/apps/vi.c) | Modal text editor | `vi file` | [Details](VI.md) |
| [`help`](../../../../userland/apps/help.c) | Command list and brief help | `help [command]` | [Details](HELP.md) |

## System and terminal

| Command | Purpose | Syntax / key options | Details |
|---|---|---|---|
| [`ps`](../../../../userland/apps/ps.c) | List processes | `ps` | — |
| [`kill`](../../../../userland/apps/kill.c) | Stop process | `kill PID` | — |
| [`pty_test`](../../../../userland/apps/pty_test.c) | Run an interactive program through a PTY | `pty_test [command [args...]]` | [PTY](../devices/PTY.md) |
| [`tty`](../../../../userland/apps/tty.c) | Show controlling terminal | `tty` | [Details](TTY.md) |
| [`stty`](../../../../userland/apps/stty.c) | Configure TTY modes | `stty [raw\|cooked\|icanon\|-icanon\|echo\|-echo\|onlcr\|-onlcr] ...` | [Details](TTY.md) |
| [`reset`](../../../../userland/apps/reset.c) | Reset terminal state | `reset` | [Details](TERMINAL.md) |
| [`lsof`](../../../../userland/apps/lsof.c) | List open file descriptors | `lsof [PID]` | — |
| [`trace`](../../../../userland/apps/trace.c) | Trace system calls and kernel events | `trace on [-s\|-a] \| off \| show [PID] \| follow [PID] \| run [-s\|-a] CMD ...` | [Details](TRACE.md) |
| [`watch`](../../../../userland/apps/watch.c) | Repeat command | `watch [seconds] command [args...]` | — |
| [`mem`](../../../../userland/apps/mem.c) | Memory viewer | `mem [address] \| mem -s [app]` | [Details](MEM.md) |

## Hardware

| Command | Purpose | Syntax / key options | Details |
|---|---|---|---|
| [`pins`](../../../../userland/apps/pins.c) | Show pin layout | `pins` | [Details](../hardware/PINS.md) |
| [`gpio`](../../../../userland/apps/gpio.c) | Interactive GPIO editor | `gpio` | [Details](../devices/README.md) |
| [`i2cctl`](../../../../userland/apps/i2cctl.c) | I²C1 device operations | `i2cctl scan \| read ADDR COUNT \| write ADDR BYTE... \| xfer ADDR READ_COUNT [BYTE...]` | [Details](../devices/I2C.md) |
| [`bh1750`](../../../../userland/apps/bh1750.c) | Read BH1750 light sensor | `bh1750 [count]— 0: continuous` | [Details](../devices/I2C.md) |
| [`fbctl`](../../../../userland/apps/fbctl.c) | Control 8×8 framebuffer | `fbctl on\|off\|clear\|status\|test` | [Details](../devices/README.md) |
| [`fbed`](../../../../userland/apps/fbed.c) | Interactive framebuffer editor | `fbed` | [Details](../devices/README.md) |
| [`pwmctl`](../../../../userland/apps/pwmctl.c) | Control PWM | `pwmctl status\|freq HZ\|a DUTY\|b DUTY\|off` | [Details](../devices/README.md) |
| [`blink`](../../../../userland/apps/blink.c) | Blink LED task | `blink [&]` | — |

## Programming and networking

| Command | Purpose | Syntax / key options | Details |
|---|---|---|---|
| [`as`](../../../../userland/apps/asm.c) | Thumb assembler (command: as) | `as file.s` | [Details](../../development/apps/asm/ASM.md) |
| [`cc`](../../../../userland/apps/cc.c) | Small C-subset compiler | `cc source.c` | [Details](../../development/apps/c/CC.md) |
| [`python`](../../../../userland/apps/python.c) | Built-in Python interpreter | `python [file]` | [Details](../../development/apps/python/PYTHON.md) |
| [`net`](../../../../userland/apps/net.c) | Wi-Fi status, clients and ARP | `net [-a\|-r]` | [Details](../network/NET.md) |
| [`ip`](../../../../userland/apps/ip.c) | Show IP interfaces | `ip [addr]` | [Details](../network/NETWORK.md) |
| [`nc`](../../../../userland/apps/nc.c) | TCP/UDP netcat | `nc [-u] [-N] -l PORT \| nc [-u] [-N] IPv4 PORT` | [Details](../network/NC.md) |
| [`ping`](../../../../userland/apps/ping.c) | ICMP Echo probes | `ping [-c COUNT] IPv4` | [Details](../network/PING.md) |
| [`tcpdump`](../../../../userland/apps/tcpdump.c) | Inspect IPv4 packets | `tcpdump [-x] [-c COUNT]` | [Details](../network/TCPDUMP.md) |
| [`httpd`](../../../../userland/apps/httpd.c) | HTTP/1.0 server (TCP 80) | `httpd` | [Details](../network/HTTPD.md) |
| [`tcp_echod`](../../../../userland/apps/tcp_echod.c) | TCP echo server | `tcp_echod [PORT]— default 7` | [Details](../network/NETWORK.md) |
| [`udp_echod`](../../../../userland/apps/udp_echod.c) | UDP echo server | `udp_echod [PORT]— default 7` | [Details](../network/NETWORK.md) |
| [`mcpd`](../../../../userland/apps/mcpd.c) | MCP HTTP server (TCP 8080) | `mcpd` | [Details](MCP.md) |
| [`scroll`](../../../../userland/apps/scroll.c) | LED ticker (stdin/UDP) | `scroll [-u [PORT]]— default UDP 2001` | [Details](SCROLL.md) |

## Demos and games

| Command | Purpose | Syntax / key options | Details |
|---|---|---|---|
| [`hello`](../../../../userland/apps/hello.c) | Print greeting | `hello [name]` | — |
| [`mb`](../../../../userland/apps/mb.c) | Moon Buggy game | `mb` | — |
| [`pong`](../../../../userland/apps/pong.c) | Pong game | `pong [-2]` | [Details](PONG.md) |
| [`reversi`](../../../../userland/apps/reversi.c) | Reversi against the computer | `reversi` | [Details](REVERSI.md) |
| [`omikuji`](../../../../userland/apps/omikuji.c) | Fortune and input demo | `omikuji [-i \| seconds]` | — |
| [`kitesys`](../../../../userland/apps/kitesys.c) | Embedded-system demo | `kitesys` | — |

## Shell built-ins

| Command | Purpose | Syntax |
|---|---|---|
| `cd` | Change directory | `cd [path]` |
| `getty` | Attach shell or Python to terminal | `getty /dev/ttyS1 sh\|python [&]` |
| `exit` | Exit the shell | `exit` |
| `logout` | Exit the shell | `logout` |

These are handled in [`shell.c`](../../../../userland/apps/shell.c), not separate PIC applications. See [PIPE.md](PIPE.md) for pipeline, redirection and background limits.

**Note:** The list of bundled commands comes from the build configuration rather than from the help text. Follow source links to verify options when the implementation changes.

[Technical references](../README.md) | [日本語](../../../ja/reference/commands/README.md)
