# PicoNIX Tiny Thumb Assembler

PicoNIX can assemble a small Cortex-M0+ Thumb-1 program on-device, write `/tmp/a.out`, and launch it in a RAM task slot. This feature is for **learning and experimentation**; it is not an ELF loader or a general-purpose toolchain.

## Quick start

From the shell (which starts in `/tmp`), run `cat > test.s`, enter the following, and press Ctrl+D to finish:

```asm
	@ Call getpid() (result returned in r0)
	movs	r0, #1		@ syscall 1: getpid
	svc	#0

	@ Explicitly terminate the program
	movs	r0, #3		@ syscall 3: exit
	movs	r1, #0		@ exit status 0
	svc	#0
```

```text
piconix# as test.s
as: 10 bytes (18 total) -> /tmp/a.out
piconix# ./a.out
```

The SVC ABI uses `r0` for the system-call number and `r1`–`r3` for arguments. Here, call 1 returns the process ID in `r0`, and call 3 explicitly terminates the program with `exit(0)`. `bx lr` returns from a subroutine to its caller; it is not used to terminate the program.

## Write to the terminal

```asm
	@ Write "Hello\n" to standard output
	movs	r0, #9		@ syscall 9: write
	movs	r1, #1		@ file descriptor 1: stdout
	adr	r2, msg		@ message address (PC-relative)
	movs	r3, #6		@ message length in bytes
	svc	#0		@ write(1, msg, 6)

	@ Exit with status 0
	movs	r0, #3		@ syscall 3: exit
	movs	r1, #0		@ exit status
	svc	#0

	.align	2		@ align msg to a 4-byte boundary
msg:
	.ascii	"Hello\n"
```

Call 9 is `write`: FD in `r1`, buffer in `r2`, length in `r3`. `.align 2` aligns the string to a word boundary for `adr`. This example produces 22 bytes of code and data (30 bytes including the header).

## Supported syntax

- `movs rN, #IMM`, `adds rN, #IMM`, `subs rN, #IMM`, `cmp rN, #IMM`: registers `r0`–`r7`, immediates 0–255
- `b LABEL`, `beq LABEL`, `bne LABEL`: PC-relative branches
- `adr rN, LABEL`: forward PC-relative word-aligned address
- `svc #IMM`, `bx lr`, `nop`
- `LABEL:`, `.align 2` (4-byte alignment), `.ascii "..."` (supports `\n`, `\r`, `\t`, `\\`)
- One instruction per line; `@` or `;` introduces comments

## Limits and safety

- `/tmp` holds up to four files of 1024 bytes each. The output has an 8-byte header, leaving at most 1016 bytes for code and data.
- Up to 32 labels; offsets and immediates must fit Thumb-1 encoding ranges.
- `as` overwrites `/tmp/a.out`; executable arguments are not implemented.
- No ELF, relocations, absolute-symbol resolution, or external linking.
- All files are volatile. The kernel copies the image to a free RAM task slot when `./a.out` is launched.
- **Untrusted machine code can corrupt memory**, since RP2040 does not provide conventional process memory protection. Run only trusted learning examples.

## Additional Thumb-1 instructions

- `mov rD, rS`: register-to-register move (the small parser accepts `r0`–`r7`)
- `ldr` / `str`: word access such as `ldr r0, [r1]` or `str r0, [r1, #4]` (byte offset 0–124, divisible by four)
- `ldrb` / `strb`: byte access (offset 0–31)
- `push {r4, lr}` / `pop {r4, pc}`: low registers plus `lr` or `pc`, respectively
- `bl label`: a short-range subroutine call within the same image
- `bcc label` / `bcs label`: unsigned conditional branches
- `.byte N`, `.word N`, `.space N`: raw byte, aligned 32-bit value, zero-filled storage

Use `adr` to get a position-independent address of in-image data before using loads and stores. Align `.word` values with `.align 2` as necessary.

Assembly success reports both the code/data byte count and the file size including the 8-byte header:

```text
as: 24 bytes (32 total) -> /tmp/a.out
```

See the [assembly examples](../../userland/examples/asm/en/README.md) for output, stack operations, and decimal conversion of numbers from 0 to 255.
