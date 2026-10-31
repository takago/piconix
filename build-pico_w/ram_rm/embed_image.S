/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.rm,"a",%progbits
.balign 4
.global piconix_rm_image_start
.global piconix_rm_image_end
piconix_rm_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_rm/rm.bin"
piconix_rm_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
