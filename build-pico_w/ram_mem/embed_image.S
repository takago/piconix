/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.mem,"a",%progbits
.balign 4
.global piconix_mem_image_start
.global piconix_mem_image_end
piconix_mem_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_mem/mem.bin"
piconix_mem_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
