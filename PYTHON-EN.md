# Python-like interpreter

[日本語](PYTHON-JP.md) | [Back to overview](README-EN.md)

`ram_apps/python.c` implements a **small PicoNIX-specific Python-like language**. It is neither CPython nor MicroPython. Applications are loaded into available RAM slots at startup.

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

## 5. Background mode and interactive UART1 session

```text
piconix# python /tmp/led.py &
piconix# python /tmp/led.py > /tmp/output.txt &
piconix# python < /dev/uart1 > /dev/uart1 &
```

The last example keeps the USB Shell available while providing a Python REPL to a terminal connected to UART1. UART1 is a byte stream; Python collects input until a line ending. Enable local echo in the UART terminal to see typed characters. CR, LF, and CRLF line endings are accepted.

**Note:** Avoid `python &` without redirecting standard input, as that would share the USB console with Shell. The console restricts input to its foreground owner. UART input does not provide Ctrl+C signal handling; use `exit()` to quit. Interactive UART operation has not yet been verified on hardware.

## Interactive use over UART1

`getty /dev/uart1 python &` attaches the Python REPL to UART1; Ctrl+C on that terminal targets its foreground task. In interactive mode, Ctrl+D does not exit the REPL: it returns to the next prompt. Enter `exit()` to quit; the UART1 session prints `PicoNIX: Session closed.`. Script-file EOF handling is unchanged.
