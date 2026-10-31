/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.asm,"a",%progbits
.balign 4
.global piconix_asm_image_start
.global piconix_asm_image_end
piconix_asm_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_asm/asm.bin"
piconix_asm_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
