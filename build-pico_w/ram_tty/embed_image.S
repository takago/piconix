/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.tty,"a",%progbits
.balign 4
.global piconix_tty_image_start
.global piconix_tty_image_end
piconix_tty_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_tty/tty.bin"
piconix_tty_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
