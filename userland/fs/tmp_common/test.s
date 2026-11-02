@ PicoNIX assembly sample: print a face 3 times.
@ r0=syscall, r1-r3=arguments; svc #0 invokes kernel.
@ 9=write(fd,buf,len), 4=msleep(ms), 3=exit(status)

        movs    r4, #3          @ Repeat count
loop:
        movs    r0, #9          @ write(1,face,6)
        movs    r1, #1
        adr     r2, face
        movs    r3, #6
        svc     #0

        movs    r0, #4          @ Sleep 1000 ms
        adr     r2, delay
        ldr     r1, [r2]
        svc     #0
        subs    r4, #1
        bne     loop

        movs    r0, #9          @ Print goodbye
        movs    r1, #1
        adr     r2, bye
        movs    r3, #7
        svc     #0

        movs    r0, #3          @ exit(0)
        movs    r1, #0
        svc     #0

        .align  2              @ 4-byte alignment
delay:
        .word   1000
face:
        .ascii  "(^_^)\n"
        .align  2              @ ADR needs alignment
bye:
        .ascii  "...bye\n"
