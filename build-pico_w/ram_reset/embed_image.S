/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.reset,"a",%progbits
.balign 4
.global piconix_reset_image_start
.global piconix_reset_image_end
piconix_reset_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_reset/reset.bin"
piconix_reset_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
