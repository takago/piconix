# help command

`help` is a standalone RAM application listing PicoNIX commands by category.

```text
piconix# help
piconix# help gpio
piconix# help as
piconix# help mb
piconix# help reset
```

The list covers registered RAM commands and shell built-ins (`cd`, `getty`, `exit`, `logout`), including `grep` and its `help grep` entry. Update the list whenever the set of commands changes. Consult the topic-specific guides for full usage and limitations. `help` is no longer a shell built-in.

## Checking pin assignments and LED output

```text
piconix# help pins
piconix# pins
piconix# help fbed
piconix# fbed
```

`pins` shows the standard physical Pico pin layout. The configured pin functions are defined in `kernel/pins.h`, and `gpio` can inspect the current pin status. `fbed` edits the 8×8 framebuffer: arrow keys move, SPACE toggles a pixel, `c` clears, and `q` quits. See [Devices](DEVICES.md) for wiring and LED scanning.

PicoNIX provides `python`, not `python3`. Run a script with `python /tmp/test.py`; `python3 tools/...` in development instructions runs on the host PC.

## Selected commands and options

- `pong -2`: two-player Pong (not a Moon Buggy option).
- `mb`: SPACE or `j` to jump, `q` to quit.
- `fbctl`: `on`, `off`, `clear`, `status`, `test`.
- `pwmctl`: `status`, `freq HZ`, `a DUTY`, `b DUTY`, `off`.
- `stty`: `raw`, `cooked`, `icanon`, `echo`, `onlcr` and their supported `-` forms.
- `kitesys`: Kanazawa Institute of Tech. Embedded System Test Program.


### echo -e / -n

`echo -e` interprets `\n`, `\r`, `\t`, `\\`, `\033` (octal), and `\x1b` (hex). `echo -n` suppresses the trailing newline. The shell has limited quoting, but `echo -e '\033[?1049l'` works.
