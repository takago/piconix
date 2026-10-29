	@ Print "Hello" and a newline to the console.
	movs	r0, #9		@ syscall 9: write
	movs	r1, #1		@ fd 1: standard output
	adr	r2, msg		@ buffer address (PC-relative, PIC)
	movs	r3, #6		@ number of bytes: "Hello\n"
	svc	#0		@ write(1, msg, 6)

	@ End the program explicitly, not via bx lr.
	movs	r0, #3		@ syscall 3: exit
	movs	r1, #0		@ status 0
	svc	#0

	.align	2		@ align the message to 4 bytes
msg:
	.ascii	"Hello\n"
