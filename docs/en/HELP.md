# help command

`help` is a standalone RAM application listing PicoNIX commands by category.

```text
piconix# help
piconix# help gpio
piconix# help as
piconix# help mb
piconix# help reset
```

The list covers all 29 registered RAM commands and shell built-ins (`cd`, `getty`, `exit`, `logout`). Consult the topic-specific guides for full usage and limitations. `help` is no longer a shell built-in.

## Verified command options

- `pong -2`: two-player Pong (not a Moon Buggy option).
- `mb`: SPACE or `j` to jump, `q` to quit.
- `fbctl`: `on`, `off`, `clear`, `status`, `test`.
- `pwmctl`: `status`, `freq HZ`, `a DUTY`, `b DUTY`, `off`.
- `stty`: `raw`, `cooked`, `icanon`, `echo`, `onlcr` and their supported `-` forms.
- `kitesys`: Kanazawa Institute of Tech. Embedded System Test Program.


### echo -e / -n

`echo -e` interprets `\n`, `\r`, `\t`, `\\`, `\033` (octal), and `\x1b` (hex). `echo -n` suppresses the trailing newline. The shell has limited quoting, but `echo -e '\033[?1049l'` works.
