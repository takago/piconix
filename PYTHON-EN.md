# PicoNIX Tiny Python — User Guide

[日本語](PYTHON-JP.md) | [Overview](README-EN.md) | [Devices](DEVICES-EN.md)

PicoNIX includes a small **Python-like interpreter written in C**, intended for learning integer expressions, control flow, and GPIO control. It is **not CPython or MicroPython**, and it does not run arbitrary standard Python programs. The behavior below is based on the current `ram_apps/python.c` implementation.

## 1. Starting and leaving the REPL

Run `python` without arguments:

```text
piconix# python
PicoNIX Tiny Python (Python subset)
Type exit() to quit.
>>> x = 10
>>> print(x + 5)
15
>>> exit()
piconix#
```

`>>>` prompts for a new statement; `...` prompts for the continuation of a block. Type **`exit()`** at top level to return to the shell.

### Multiline blocks

Terminate `if`, `while`, and `for` headers with `:` and indent their bodies using spaces. In the interactive REPL, **enter a blank line to finish the block**:

```text
>>> for i in range(3):
...     print(i)
...
0
1
2
```

The block runs once entry is complete. Pasting multiple lines quickly may visually interleave TTY echo with `...` prompts. This does not necessarily mean that input was lost. For larger examples, use a script file.

## 2. Running a script file

Temporary files live in RAM under `/tmp`. The shell starts in `/tmp` by default, so relative paths normally suffice:

```text
piconix# pwd
/tmp
piconix# cat > test.py
print(1 + 2)
```

Press `Ctrl+D` on an empty line to finish `cat`, then execute:

```text
piconix# python test.py
3
```

`python /tmp/test.py` also works. Files in `/tmp` are volatile and disappear after reboot. The interpreter can **load** a script file, but Python-level `open()`, `read()`, and `write()` file APIs are **not implemented**.

## 3. Supported syntax

| Area | Supported features |
|---|---|
| Values | Signed 32-bit integers, `True` (1), `False` (0) |
| Variables | Integer assignments such as `x = 123` |
| Arithmetic | `+`, `-`, `*`, `/`, `%`, parentheses, unary `+` and `-` |
| Comparisons | `==`, `!=`, `<`, `<=`, `>`, `>=` |
| Logic | `not`, `and`, `or` (short-circuit evaluation) |
| Branches | `if condition:`, `else:` |
| Loops | `while condition:`, `for name in range(...):` |
| Loop control | `break`, `continue` |
| Comments | `#` to the end of the line |

`True` and `False` are integer constants, not a distinct Boolean type. Zero is false; nonzero integers are true. As in Python, `and` and `or` can return an **operand value**, rather than a normalized Boolean.

```python
x = 7
if x >= 5 and not False:
    print("OK")
else:
    print("NG")
```

Expression precedence is roughly `* / %`, then `+ -`, comparisons, `not`, `and`, and `or`. `and` and `or` skip evaluating unnecessary right-hand operands. Division is integer division; division by zero is an error.

### for, range, break and continue

```python
for i in range(5):
    print(i)

for i in range(5, 0, -2):
    print(i)
```

The accepted forms are `range(stop)`, `range(start, stop)`, and `range(start, stop, step)`. The stop is exclusive; negative steps are supported, while a zero step is an error.

```python
for i in range(10):
    if i == 2:
        continue
    if i == 5:
        break
    print(i)
```

`break` and `continue` target the innermost loop. Each executed `for` or `while` loop is limited to 10,000 iterations; `while True:` is therefore not unlimited.

## 4. Built-in I/O and delays

| Expression | Behavior / limits |
|---|---|
| `print(123)` | Print the value of an integer expression |
| `print("hello")` | Print a string literal |
| `input("number? ")` | Read a line and return a parsed integer |
| `int(input())` | Accepted, although `input()` already returns an integer |
| `msleep(500)` | Sleep for 500 ms (allowed range: 0–60,000) |

`print()` accepts **only one argument**. String variables, concatenation, and multi-argument forms such as `print("x=", x)` are unsupported. `input()` does **not** return a string; it reads a number. Invalid numeric input or expressions result in an error.

```python
n = input("Number: ")
print(n * 2)
```

## 5. GPIO operations

GPIO is accessed through `/dev/gpio/N` and its configuration devices, not through a dedicated GPIO system call.

| Function | Purpose |
|---|---|
| `gpio_mode(pin, 0)` | Set input direction |
| `gpio_mode(pin, 1)` | Set output direction |
| `gpio_pull(pin, 0)` | Disable internal pulls |
| `gpio_pull(pin, 1)` | Enable pull-up |
| `gpio_pull(pin, 2)` | Enable pull-down |
| `gpio(pin, 0)` / `gpio(pin, 1)` | Drive LOW / HIGH (also configures output direction) |
| `gpio_read(pin)` | Return digital input 0 or 1 |

The implementation accepts GPIO numbers in the range 0–28 but **excludes GP0, GP1, GP4 and GP5** from Python access. Pins can also be occupied by peripherals such as the framebuffer; consult the [device guide](DEVICES-EN.md) and the physical board pinout.

### Blink an LED

Connect an LED with a suitable current-limiting resistor to GP16:

```python
gpio_mode(16, 1)
for i in range(10):
    gpio(16, True)
    msleep(250)
    gpio(16, False)
    msleep(250)
```

### Read a switch using a pull-up

For a switch between GP16 and GND, configure **both input direction and an internal pull-up**. Pressing the switch then reads LOW (0):

```python
gpio_mode(16, 0)
gpio_pull(16, 1)

for i in range(10):
    if gpio_read(16):
        print("OFF")
    else:
        print("ON")
    msleep(500)
```

Without a pull-up or pull-down, a disconnected switch input may float. If a program only starts seeing sensible values after `kitesys` runs, check whether `kitesys` left a pull configuration in place.

## 6. Background execution and a second terminal

```text
piconix# python test.py &
piconix# python test.py > output.txt &
piconix# getty /dev/uart1 python &
```

The last command attaches the Python REPL to UART1. In PicoNIX, `getty` is a command that starts a program on a specified TTY, **not a persistent daemon**. Use `exit()` to close the session. Avoid launching multiple interactive programs on the same TTY at once, as they can compete for input.

The REPL generally uses TTY line input. Terminal-side line editing or echo options may interfere with the device's own input handling. When pasting multiple lines at speed, `>>>` and `...` output can mix with echoed text even if no bytes are lost.

## 7. Limits and troubleshooting

| Limit | Current value |
|---|---|
| Source text | Up to 512 bytes |
| Effective source lines | Up to 64 |
| Variables | Up to 16; names up to 15 characters |
| Execution nesting | Up to 8 blocks |
| Loop iterations | Up to 10,000 per executed loop |
| One REPL input line | Up to 127 stored characters |

Script loading is limited to 512 bytes. Each submitted REPL input group also has a 512-byte limit; separate submissions can be made interactively. This is a small language subset: `def`, `import`, lists, dictionaries, floating-point values, Python file APIs, string variables, and exceptions are not implemented. Use spaces, not tabs, for block indentation.

On parse or execution failures, the interpreter generally prints `python: syntax/runtime error` rather than detailed exceptions. Check syntax, indentation, variable names, pin numbers, iteration limits, and source length.

## 8. Implementation

The interpreter lives in [`ram_apps/python.c`](ram_apps/python.c). Its integer expression parser, block executor, REPL, and GPIO device access are deliberately kept in one file. When extending it, also verify the 32 KiB RAM application-slot limit.
