# Educational C-subset compiler (`cc`)

*Development and language reference (current implementation: `userland/apps/cc.c`)*

`cc` is a small C-subset compiler that runs on PicoNIX. It translates a `.c` file into PicoNIX-specific Thumb-1 assembly (`.s`), which the built-in `as` converts into an executable. **It is not ISO C or GCC compatible.** This page describes the supported language and its limitations.

## Compile and run

Create `/tmp/hello.c` with `edit` or use `cat > /tmp/hello.c` followed by Ctrl+D:

```c
int main(void) {
    write(1, "Hello, PicoNIX!\n", 16);
    return 0;
}
```

```sh
cc /tmp/hello.c
cat /tmp/hello.s
as /tmp/hello.s
/tmp/a.out
```

The compiler replaces the final `.c` with `.s` **in the same directory**. `as` writes its binary to `/tmp/a.out`. **Never execute an old `/tmp/a.out` after a failed compile or assembly.** Inspect the `.s` file to study branches and `svc #0`.

## Supported syntax

| Feature | Current subset |
|---|---|
| Functions | Required `int main()` or `int main(void)`; definitions and calls of `int`/`void` functions; recursive code generation and forward calls; up to 16 definitions and 32 call sites |
| Parameters and returns | Up to four parameters of types `int`, `int *`, or `char *`; `return expression;` in `int` functions and `return;` in `void` functions; no pointer return types |
| Locals | 32-bit `int`, one-byte `char`, one-dimensional arrays, and single-level pointers; up to 16 local names and 256 bytes of frame space per function |
| Arrays | Indexed loads/stores `a[i]`, `sizeof(a)`, and `char s[6] = "HELLO";`; lengths 1–64 elements, no bounds checking |
| Expressions | Variables, integer/character literals, parentheses, unary `+`/`-`, binary `+`/`-`/`*`/`/`/`%`, `sizeof(local)` |
| Conditions | **Signed 32-bit** `==`, `!=`, `<`, `<=`, `>`, `>=` comparisons in `if`/`else`/`while`; braces required around controlled statements |
| Pointers | `&x`, `&a[i]`, `*p`, `*p = value`, `p[i]`, `p + n`, `p - n`; arrays passed as pointer arguments; `int *` arithmetic scales by four bytes, `char *` by one |
| Increment/decrement | Prefix and postfix `++`/`--` on **scalar local `int` variables** only (statements and expressions) |
| Built-ins | `getpid();`, `read(fd, buffer, count)`, `write(fd, buffer, count)`; integer expressions for FD/count, and compatible local arrays, local addresses or pointers for buffer |
| Comments | `//` and `/* ... */` |

Positive decimal literals range from `0` through `2147483647`. Negative expressions use unary `-`. C-style overflow behavior is not guaranteed. Character literals support a small set of escapes.

### Pointers and functions

```c
void swap(int *a, int *b) {
    int t;
    t = *a;
    *a = *b;
    *b = t;
}
int main(void) {
    int x = 10;
    int y = 20;
    swap(&x, &y);
    if (x == 20) { write(1, "OK\n", 3); }
    return 0;
}
```

Save as `/tmp/swap.c`, then run `cc /tmp/swap.c`, `as /tmp/swap.s`, and `/tmp/a.out` in order. Use `&a[i]` for an array-element address; `read(0, &c, 1)` also accepts the address of a scalar `char` local.

### Arrays and dynamic write lengths

```c
int main(void) {
    char buf[16];
    int n = read(0, buf, sizeof(buf));
    if (n > 0) { write(1, buf, n); }
    return 0;
}
```

Use the return value of `read` rather than writing uninitialized bytes. The compiler does not check runtime buffer lengths.

### Conditional branches and `++`/`--`

```c
int main(void) {
    int i = 0;
    while (i < 3) {
        if (i <= 1) { write(1, "A\n", 2); }
        else { write(1, "B\n", 2); }
        i++;
    }
    return 0;
}
```

Prefix increments/decrements yield the updated value, while postfix forms yield the previous value. `++`/`--` on `char`, pointer variables, array elements, or `*p` is **not** supported.

## String literals and system calls

`write(1, "HELLO", 3);` uses a special literal form requiring a numeric FD in the range 0–2 and a nonnegative numeric count no larger than the string's actual byte length. It writes `HEL`, not the entire literal. Escapes `\n`, `\r`, `\t`, and `\\` are supported; escaped quotes (`\"`) are not.

In `write(fd, buf, n)` and `read(fd, buf, n)` the FD and count may be integer expressions, and the buffer may be a local array, `&` of a scalar local, or a compatible pointer expression. A `read` result can be assigned to a variable. There is no general runtime bounds checking.

## Unsupported features and limits

- **Not supported:** preprocessor or `#include`, arbitrary library functions, globals, `struct`/`union`/`enum`, `typedef`, `const`/`static`, `for`, `switch`, `do`, `&&`/`||`, ternary operators, casts, compound assignment, prototypes without definitions, and general lexical block shadowing.
- **Pointer restrictions:** no multiple indirection (`int **`), arrays of pointers, pointer returns, pointer-pointer subtraction, function pointers, or `malloc()`. Pointer and array indexes are unchecked.
- **Division and remainder:** repeated-subtraction implementation intended for **nonnegative** operands only. Division by zero returns zero in this implementation; do not rely on it as C behavior.
- **Size limits:** input C source ≤1024 bytes; internal generated-assembly buffer ≤4096 bytes, but **the `.s` RAM file is limited to 2048 bytes**. The assembler's machine-code payload is limited to 2040 bytes and 32 labels. Constants and calls can exhaust these limits quickly.
- **Safety:** PIC applications lack memory protection. Invalid addresses can corrupt memory; compile errors are not always descriptive, and complete ABI compatibility is not guaranteed.

## How the compiler and assembler work

Expressions are parsed by recursive descent (`primary` → `unary` → `multiply` → `additive`); intermediate values are saved with `push`/`pop`, with expression results in `r0`. Up to four call arguments use `r0`–`r3`, and function return values use `r0`. Each function saves `{r4, lr}`, reserves a fixed 256-byte local frame, and uses `r4` as its frame pointer. The entry stub passes `main`'s return value to the `exit` syscall; **return does not always mean process exit**, since user functions can return to their callers.

The syscall numbers for `read`, `write`, and `exit` are 8, 9, and 3. The number is passed in `r0`, arguments in `r1`–`r3`, and `svc #0` performs the call. The zero immediate in `svc` is *not* the syscall number. Generated instructions use two spaces of indentation; labels and directives start at column zero.

## Tests and related references

```sh
python3 tools/test_scripts/test_cc_regressions.py
python3 tools/test_scripts/test_cc_functions.py
python3 tools/test_scripts/test_cc_pointers.py
python3 tools/test_scripts/test_cc_increment.py
```

These are **host-side C-to-Thumb generation/assembly tests**, not proof of execution on RP2040. On hardware, confirm each of compilation, assembly and actual output separately.

[Assembler](../asm/ASM.md) | [Syscalls](../../../reference/SYSCALLS.md) | [日本語](../../../../ja/development/apps/c/CC.md)
