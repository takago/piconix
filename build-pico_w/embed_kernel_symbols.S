.section .rodata.piconix_kernel_symbols,"a",%progbits
.balign 4
.global piconix_kernel_syms_start
.global piconix_kernel_syms_end
piconix_kernel_syms_start:
.incbin "/tmp/PicoNIX/build-pico_w/piconix_kernel_symbols.bin"
piconix_kernel_syms_end:
.balign 4
.section .note.GNU-stack,"",%progbits
