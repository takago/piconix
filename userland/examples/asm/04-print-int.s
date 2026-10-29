	@ Print the current PID as decimal (0..255).
	movs	r0, #1		@ syscall 1: getpid
	svc	#0		@ r0 = PID
	bl	print_int	@ bl saves return address in lr
	movs	r0, #3		@ syscall 3: exit
	movs	r1, #0		@ 0
	svc	#0

	@ print_int(r0=0..255): print decimal.
print_int:
	push	{r4,r5,r6,r7,lr}
	mov	r4, r0
	movs	r5, #0

	@ Divide by repeated subtraction.
hundreds:
	cmp	r4, #100
	bcc	tens_init
	subs	r4, #100
	adds	r5, #1
	b	hundreds

tens_init:
	movs	r6, #0

tens_loop:
	cmp	r4, #10
	bcc	write_digits
	subs	r4, #10
	adds	r6, #1
	b	tens_loop

	@ ASCII digits: 48 = '0'.
write_digits:
	adr	r2, buffer
	mov	r7, r2
	movs	r3, #0		@ digit count
	cmp	r5, #0
	beq	check_tens
	adds	r5, #48
	strb	r5, [r7]
	adds	r7, #1
	adds	r3, #1

check_tens:
	cmp	r3, #0
	bne	write_tens
	cmp	r6, #0
	beq	write_units

write_tens:
	adds	r6, #48
	strb	r6, [r7]
	adds	r7, #1
	adds	r3, #1

write_units:
	adds	r4, #48
	strb	r4, [r7]
	adds	r3, #1
	movs	r0, #9		@ write(1, buffer, len)
	movs	r1, #1
	svc	#0
	pop	{r4,r5,r6,r7,pc}	@ return

	.align	2
buffer:
	.space	3
