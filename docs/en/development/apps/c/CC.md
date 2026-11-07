## 2026-11-07: Basic pointers (new)

`cc` now supports single-level `int *` and `char *` locals/parameters, `&x`, `&a[i]`, `*p`, `*p = value`, `p[i]`, `p + n`, and `p - n`. Arrays decay to addresses when passed to functions. Arithmetic scales by 4 bytes for `int *`, and by 1 byte for `char *`. Buffer expressions for `read()` and `write()` may be pointers.

```c
void swap(int *a, int *b)
{
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

int main(void)
{
    int x;
    int y;
    x = 10;
    y = 20;
    swap(&x, &y);
    if (x == 20) {
        if (y == 10) { write(1, "PASS\n", 5); }
    }
    return 0;
}
```

Build with `cc test.c`, `as test.s`, then `/tmp/a.out`.

**Limitations:** no multiple indirection (`int **`), arrays of pointers, structs, pointer-pointer subtraction, pointer return types, `++`/`--`, `malloc()`, or function pointers. There are no bounds checks, so invalid pointers are unsafe. Each function reserves a 256-byte frame; source input is limited to 1024 bytes and assembler source to 2048 bytes. Host-side code-generation and assembly tests pass; hardware execution is unverified.

## 2026-11-07: Local address-of and larger integer constants

The compiler now supports `&c` for a scalar local (`char c;`, `int n;`). `read` and `write` accept a named local array or the address of a scalar local as their buffer argument. One-element arrays, such as `char c[1];`, remain arrays.

```c
int main(void)
{
    char c;
    int n;
    n = read(0, &c, 1);
    if (n == 1) {
        write(1, &c, 1);
    }
    return 0;
}
```

Positive decimal literals through 2147483647 are accepted. When an integer does not fit Thumb-1 `movs`, the compiler builds its bit pattern using a sequence of `adds` instructions. Unary `-` handles negative expressions. The following sample now compiles:

```c
void print_number(int n)
{
    char digit[1];
    if (n >= 10) {
        print_number(n / 10);
    }
    digit[0] = '0' + n % 10;
    write(1, digit, 1);
}

int main(void)
{
    print_number(12345);
    write(1, "\n", 1);
    return 0;
}
```

`cc` replaces the source `.c` extension with `.s` in the same directory; `as` writes `/tmp/a.out`. General pointer variables, pointer arithmetic, and array function parameters remain unsupported. Array indexing is unchecked. Large constants produce more instructions, so beware the 4096-byte compiler output limit and the 2048-byte assembler source limit. RP2040 execution has not been verified for this change.

> **Current implementation and tests (2026-11-07):** The compiler uses a fixed 256-byte local frame and supports a subset of `int`, `char`, arrays, `sizeof`, and dynamic `read(fd,buf,n)`/`write(fd,buf,n)`. Some older sections below describe previous 64-byte limitations; use this note and source as the current reference. Run `python3 tools/test_scripts/test_cc_regressions.py` and the existing `test_cc_*.py` tests. These validate C-to-Thumb encoding, not execution on RP2040.

# PicoNIX Educational C Compiler (`cc`)

`cc` is a deliberately tiny C-subset compiler running on PicoNIX. It emits Thumb-1 assembly for the built-in `as`, not general C or GNU assembly.

## First exercise

Create `/tmp/hello.c` using `cat > /tmp/hello.c`, then Ctrl+D:

```c
int main(void) {
    write(1, "Hello, PicoNIX!\n", 16);
    return 0;
}
```

The example uses the standard **three-argument** `write(fd, buffer, count)` calling convention. This compiler only supports a string literal for `buffer` and an integer constant for `count`. The count must not exceed the literal byte length.

```sh
cc /tmp/hello.c
cat /tmp/hello.s
as /tmp/hello.s
./a.out
```

`cc` writes assembly beside the input source (`cc test.c` → `test.s`; `cc /tmp/hello.c` → `/tmp/hello.s`). `as` still writes `/tmp/a.out`.

## Supported subset

One `int main(void)` or `int main()` function; statements `write(fd, "literal", count);` (fd 0–2; count 0 through literal byte length), `getpid();`, and `return N;` (0–255); comments and multiple statements. The `getpid()` result is discarded.

Not supported: general arithmetic expressions, `else`, `>`/`<=`, arrays, pointers, other functions, `#include`, or pointer-based `write`.

## How it works

The compiler recognizes a restricted grammar and emits `movs`, `adr`, `svc #0`, branches, and aligned `.ascii` data. The syscall number is in `r0` (`write=9`, `exit=3`); the syscall arguments occupy `r1`–`r3`. `adr` makes string references position independent. The assembler creates a PNX1 image for the shell to run.

## Limits and safety

Source: 1024 bytes. Assembly: 2048 bytes. `as` image payload: 2040 bytes and at most 32 labels. Literal strings support ordinary printable ASCII and `\n`, `\r`, `\t`, `\\`; escaped double quotes are not supported. The compiler does not provide complete C diagnostics. PicoNIX has no memory protection; run trusted experiments only.

Inspect `sample.s` to see how variables, comparisons and loops map to Thumb-1 instructions.

See [Assembler](../asm/ASM.md) and [Syscalls](../../../reference/SYSCALLS.md).

## Loop exercise

```c
int main(void) {
    int i = 0;
    while (i < 3) {
        write(1, "HI\n", 3);
        i = i + 1;
    }
    if (i == 3) { write(1, "OK\n", 3); }
    return 0;
}
```

Compile with `cc /tmp/loop.c`, inspect `/tmp/loop.s`, assemble with `as /tmp/loop.s`, and run `./a.out`. The subset does not support `else`, general expressions, signed arithmetic, pointers, or function calls. Constants and comparisons are restricted to 0–255; arithmetic overflow and general C ABI guarantees are not provided.

## Additional control flow

`if (condition) { ... } else { ... }` is now supported, as are `>` and `<=` comparisons. Conditions are still **unsigned**, not general signed C `int` comparisons. The compiler implements the additional operators using existing Thumb-1 conditional and unconditional branches. Blocks remain mandatory.


## Stack-based locals (2026-11-07)

`cc` reserves a fixed 64-byte stack frame with `sub sp, #64`, allowing 16 32-bit local integers at offsets 0, 4, ... 60. Loads and stores use `ldr` and `str` with SP-relative addressing. The assembler now supports `sub/add sp, #N` (multiples of four up to 508) and `ldr/str rN, [sp, #offset]` (multiples of four up to 1020). Existing register-relative addressing remains supported.

This is a restricted teaching compiler: general expressions, lexical scopes, function calls, signed comparisons and dynamic stack frames are not implemented. `return` invokes the PicoNIX exit syscall rather than returning from an ordinary C function.

## Recursive-descent expressions (2026-11-07)

The top-down grammar is `expression → additive → multiply → unary → primary`. Parenthesized expressions, integer literals, local variables, unary `+`/`-`, binary `+`/`-`/`*`/`/`/`%`, and expressions on both sides of comparisons are supported. The parser preserves intermediate results using `push {r0}` and `pop {r1}`. Thumb `muls` performs multiplication; repeated subtraction implements a deliberately simple division/remainder algorithm.

```c
int main(void) {
    int a=10; int b=3; int c;
    c=(a+b*2)/4;
    if(c==4) { write(1,"OK\n",3); }
    return 0;
}
```

Run `cc /tmp/example.c`, `as /tmp/example.s`, then `./a.out`. **Limitations:** division and remainder currently support nonnegative operands only; division by zero returns zero rather than implementing standard C semantics. Comparisons are unsigned. Output size, branch range, integer overflow, and expression nesting remain constrained. Hardware execution has not been verified. Host check: `python3 tools/test_scripts/test_cc_expressions.py`.

## `char` and arrays (experimental)

`int` is 32-bit and `char` occupies one byte. Fixed-size local arrays reside in a 256-byte stack frame. Up to 16 local declarations are accepted. Indexed loads/stores, `sizeof(local)`, character constants, and `char msg[6] = "HELLO";` initialization are supported.

```c
int main(void) {
    char buf[8];
    int n = read(0, buf, 8);
    if (n > 0) write(1, buf, 8);
    return 0;
}
```

**Limitations:** Array-based `read`/`write` accept expressions for their third argument, including `write(1, buf, n)`. Array indexing is unchecked. Pointers, general function calls, block scoping, and standard-conforming `char` signedness are not supported. The generated code uses `addsp` (new assembler instruction), `ldrb`/`strb` and `ldr`/`str`. Run `python3 tools/test_scripts/test_cc_char_arrays.py` for the host parser/assembler tests. Hardware execution is unverified.

## Dynamic syscall arguments (2026-11-07)

```c
int main(void)
{
    char buf[32];
    int n = read(0, buf, sizeof(buf));
    if (n > 0) {
        write(1, buf, n);
    }
    return 0;
}
```

For `read(fd, array, count)` and `write(fd, array, count)`, the file descriptor and count may be integer expressions. The buffer must be a named local array, not a general pointer. Arguments are evaluated left to right and temporarily saved on the stack before being restored to `r1` (fd), `r2` (buffer), and `r3` (count). `r0` selects the syscall for `svc #0`; a `read` return value remains in `r0`.

There is no runtime bounds check for the requested count. Test with `python3 tools/test_scripts/test_cc_syscall_args.py`. RP2040 execution remains unverified.

### Signed comparisons (2026-11-07)

The compiler now emits signed conditional branches (`blt`, `ble`, `bgt`, `bge`) after `cmp`; the assembler encodes the corresponding Thumb-1 conditions. Run `python3 tools/test_scripts/test_cc_signed_comparisons.py` to verify assembly generation and binary encodings. This is not a CPU execution test. Signed division/remainder and on-device execution remain unverified.


## User-defined functions (experimental)

Functions returning `int` or `void`, up to four `int` parameters, per-call local variables, nested calls, and recursive code generation are supported. Arguments use `r0`–`r3`, results use `r0`, and the prologue saves `lr` and reserves a 256-byte local frame. Forward calls to functions defined later are supported; standalone function prototypes are not.

```c
int add(int a, int b) {
    return a + b;
}
int main(void) {
    return add(10, 20);
}
```

Run `cc sample.c`, `as sample.s`, and `./a.out`. Limits: 16 functions, 32 call sites, and `int` parameters only; pointer/array parameters are not implemented. Stack alignment across nested calls is not fully AAPCS-compliant. Recursive code assembles, but runtime behavior is unverified. `tools/test_scripts/test_cc_functions.py` checks code generation and assembly, not CPU execution.

## Generated assembly indentation

The generated `.s` file uses two leading spaces for instructions. Labels and assembler directives such as `.align` and `.ascii` stay at column zero. The assembler ignores leading whitespace, so the machine code is unchanged. The extra whitespace does count toward the assembler source-size limit.

```asm
f_add:
  push {r4, lr}
  sub sp, #256
  ldr r0, [sp, #0]
  add sp, #256
  pop {r4, pc}
```

### Stable local-variable addressing across function calls (2026-11-07)

The compiler uses the callee-saved `r4` register as a frame pointer for parameters and locals. Temporary pushes during argument evaluation no longer shift local-variable addresses. This fixes nested calls such as `square(sum)` after `sum=add(a,b)`. Hardware execution remains unverified.
