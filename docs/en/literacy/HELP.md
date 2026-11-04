# help command

`help` is a standalone RAM application listing PicoNIX commands by category.

```text
piconix# help
piconix# help gpio
piconix# help as
piconix# help mb
piconix# help reset
piconix# help cat
```

The list covers registered RAM commands and shell built-ins (`cd`, `getty`, `exit`, `logout`), including `grep` and its `help grep` entry. Update the list whenever the set of commands changes. Consult the topic-specific guides for full usage and limitations. `help` is no longer a shell built-in.

## Checking pin assignments and LED output

```text
piconix# help pins
piconix# pins
piconix# help fbed
piconix# fbed
```

`pins` shows the standard physical Pico pin layout. The configured pin functions are defined in `kernel/pins.h`, and `gpio` can inspect the current pin status. `fbed` edits the 8×8 framebuffer: arrow keys move, SPACE toggles a pixel, `c` clears, and `q` quits. See [Devices](../reference/devices/DEVICES.md) for wiring and LED scanning.

PicoNIX provides `python`, not `python3`. Run a script with `python /tmp/test.py`; `python3 tools/...` in development instructions runs on the host PC.

## Reading files and checking line numbers

`cat FILE` shows the file as it is; `cat -n FILE` shows every line with a number, starting at 1. Blank lines are numbered too. With no FILE, `cat` reads standard input, so `echo hello | cat -n` also works. Use `help cat` to see the command format and [Standard I/O lab](IO-IPC.md) for a short exercise.

Some other important usage details: `ls -a` shows `.` and `..`; `ls -l` shows type, ID and size. `edit` accepts RAM files directly under `/tmp` or `/etc`. `dd` accepts `if=` and `of=` (or defaults to stdin/stdout), with `bs=1..256`; `grep` matches literal substrings and skips input lines longer than 128 bytes.

## Selected commands and options

- `pong -2`: two-player Pong (not a Moon Buggy option).
- `mb`: SPACE or `j` to jump, `q` to quit.
- `fbctl`: `on`, `off`, `clear`, `status`, `test`.
- `pwmctl`: `status`, `freq HZ`, `a DUTY`, `b DUTY`, `off`.
- `stty`: `raw`, `cooked`, `icanon`, `echo`, `onlcr` and their supported `-` forms.
- `kitesys`: Kanazawa Institute of Tech. Embedded System Test Program.


### echo -e / -n

`echo -e` interprets `\n`, `\r`, `\t`, `\\`, `\033` (octal), and `\x1b` (hex). `echo -n` suppresses the trailing newline. The shell has limited quoting, but `echo -e '\033[?1049l'` works.

## Pico W network commands

`ping [-c COUNT] IPv4` sends four Echo Requests by default; use a numeric address such as `192.168.4.2`. `tcpdump [-x] [-c COUNT]` shows IPv4 TCP/UDP/ICMP packet summaries; use `-x` to show the data length and up to 16 payload bytes (HEX and ASCII) on an indented second line. These commands require Pico W. See the [networking lab](NETWORKING.md).

`ip addr` displays the loopback and Wi-Fi interfaces on Pico W. For `edit`, press Ctrl+Q twice to discard changes (or Ctrl+S to save); Ctrl+C also aborts without saving.
