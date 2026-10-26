# Python-like interpreter

[日本語](PYTHON-JP.md) | [Back to overview](README-EN.md)

`ram_apps/python.c` implements a **small PicoNIX-specific Python-like language**. It is neither CPython nor MicroPython. It shares RAM slot 3 with Omikuji, so they cannot run together.

## 1. Interactive mode

```text
piconix# python
>>> x = 10
>>> print(x + 5)
15
>>> exit()
piconix#
```

Run `python` without arguments to enter the REPL; type `exit()` to return to Shell. Input is line-based. Indent `if` and `while` bodies, and finish a block with an empty line.

## 2. Run a script

Create `/tmp/test.py` using:

```text
piconix# cat > /tmp/test.py
print(1 + 2)
```

Finish by pressing `Ctrl+D` on an empty line, then run:

```text
piconix# python /tmp/test.py
3
```

After `cd /tmp`, `python test.py` also works. Only RAM scripts under `/tmp` are supported, and they disappear on reboot. Script mode suppresses the interactive banner.

## 3. Supported features

| Category | Features |
|---|---|
| Values and math | Signed 32-bit integers, `+ - * / %`, comparisons, parentheses |
| Control | Assignment, `if`/`else`, `while`, space indentation, `#` comments |
| I/O | `print(...)`, integer-valued `input(...)`, `int(input(...))` |
| Delays | `sleep(milliseconds)` (0–60000) |
| GPIO | `gpio`, `gpio_read`, `gpio_mode`, `gpio_pull` |

Limits: 16 variables, 512 source bytes, 64 nonempty lines, and eight nested blocks. Each `while` statement is capped at 10,000 iterations. There are no floats, lists, user-defined functions, `for`, imports, or string variables.

## 4. GPIO example

Save the following as `/tmp/led.py` and run `python /tmp/led.py`:

```python
i = 0
while i < 5:
    gpio(25, 1)
    sleep(250)
    gpio(25, 0)
    sleep(250)
    i = i + 1
```

`gpio_mode(pin, 0)` selects input and `gpio_mode(pin, 1)` output. `gpio_pull(pin, 0/1/2)` selects none/pull-up/pull-down. GPIO 0, 1, 4, and 5 are reserved for UART connections. Observe electrical limits for the board and attached circuits.

## 5. Background mode

```text
piconix# python /tmp/led.py &
piconix# python /tmp/led.py > /tmp/output.txt &
```

`python &` is unsupported because an interactive interpreter would compete with Shell for console input. Background scripts also have restrictions on console reads.
