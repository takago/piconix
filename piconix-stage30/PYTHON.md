# Tiny Python-style interpreter (slot 3)

PicoNIX includes a small, freestanding interpreter in `ram_apps/python.c`.
It shares the **third physical task slot** (`0x20030000`) with `omikuji`:
they cannot run simultaneously, but both commands remain available.

Run `python` with no arguments for an interactive REPL. The banner is shown
only in REPL mode:

```text
piconix# python
PicoNIX Tiny Python (Python subset)
Type exit() to quit.
>>> x=10
>>> print(x+5)
15
>>> exit()
piconix#
```

Type `exit()` to return to the Shell. Running `python /tmp/test.py` executes
the script **without a banner**, so redirected program output is not polluted. An expression such as `1+2+3` prints its value. A control block
(`if` or `while`) is entered with the `...` prompt and terminated by an empty
line. The REPL uses the existing line-based console input.

Create a script with `cat > /tmp/test.py` (Ctrl+D to finish), then run
`python /tmp/test.py`. From `/tmp`, `python test.py` also works. Only `/tmp`
regular files are supported (512 bytes per file). No arguments means REPL;
there is no longer an implicit `/tmp/python.py` default.

Example (keep under the 512-byte per-file limit):

```python
n = int(input("Count: "))
i = 0
while i < n:
    if i % 2 == 0:
        print(i)
    i = i + 1
print("done")
```

## Supported subset

- Signed 32-bit integer variables (16 maximum), integer constants, arithmetic
  (`+ - * / %`), parentheses and six comparisons.
- Assignment, `if`/`else`, `while`, `#` comments and space-indented blocks.
- `print(integer expression)` and `print("literal")`; `input("prompt")`
  accepts an integer (not a Python string); `int(input(...))` also works.
- `sleep(milliseconds)` pauses the current Python task (0–60000 ms).
- `gpio(pin, value)` selects output mode then writes 0/1 to GPIO; `gpio_read(pin)`
  reads the pin's current level. Uses `/dev/gpio/N` and `/dev/gpio/N.dir`.
- `gpio_mode(pin, mode)` chooses input (0) or output (1) using `/dev/gpio/N.dir`.
- `gpio_pull(pin, mode)` chooses none (0), pull-up (1), pull-down (2) using
  `/dev/gpio/N.pull`.
  Pins 0, 1, 4, 5 (reserved by console/UART) and pins outside 0–28 are rejected.
- Up to 64 nonempty source lines, 512 bytes total, and eight nested blocks.
  Infinite loops are limited to 10,000 iterations per `while` statement.

This is a deliberately constrained Python-like language, **not CPython or MicroPython**.
It has no lists, strings as variables, `for`, functions, imports, floating point,
Python's complete expression/indentation semantics.
Input is line-based through the existing canonical console interface.

The existing GETARG syscall can now copy a spawned Python script pathname; its number is unchanged. RAM files disappear on reboot.

GPIO example (save as `/tmp/led.py`):

```python
i = 0
while i < 5:
    gpio(25, 1)
    sleep(250)
    gpio(25, 0)
    sleep(250)
    i = i + 1
```

`gpio_read(N)` returns 0 or 1, e.g. `print(gpio_read(16))`.

## Examples and practical notes

The following scripts may be pasted into `cat > /tmp/name.py`; press Ctrl+D on an empty line to finish. Scripts are limited to 512 bytes. REPL expressions print their values, but for file scripts use `print(...)` to display a result.

```python
n = int(input("Blink count: "))
i = 0
while i < n:
    gpio(25,1)
    sleep(200)
    gpio(25,0)
    sleep(200)
    i = i + 1
print("done")
```

For an input with pull-up enabled, `gpio_mode(16,0)` and `gpio_pull(16,1)` configure GPIO16 as an input with pull-up; `gpio_read(16)` reads the current level. Avoid driving external loads directly from a GPIO pin.

There is no package loader, general-purpose string handling or filesystem beyond `/tmp`. The `python` command launches the interpreter in the shared slot 3 and cannot run at the same time as `omikuji`. A background program using standard console input may be refused by the console's foreground-task check.

### Background execution

A script may run in the background with `&`, including with output redirection:

```sh
python /tmp/test3.py &
python /tmp/test3.py > /tmp/out.txt &
```

`python &` is not supported because an interactive REPL would compete with the shell for console input.

