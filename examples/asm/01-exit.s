	@ Exit the program normally with status 0.
	@ PicoNIX SVC ABI: r0 = call number, r1 = argument 1.
	movs	r0, #3		@ syscall 3: exit
	movs	r1, #0		@ exit status: success
	svc	#0		@ enter kernel (does not return)
