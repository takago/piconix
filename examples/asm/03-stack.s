	@ Demonstrate saving/restoring a register on the stack.
	movs	r4, #7		@ initial value
	push	{r4}		@ save r4; stack grows downward
	movs	r4, #0		@ overwrite r4
	pop	{r4}		@ restore r4 (now 7 again)

	movs	r0, #3		@ syscall 3: exit
	movs	r1, #0		@ normal exit
	svc	#0
