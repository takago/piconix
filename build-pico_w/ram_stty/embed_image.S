/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.stty,"a",%progbits
.balign 4
.global piconix_stty_image_start
.global piconix_stty_image_end
piconix_stty_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_stty/stty.bin"
piconix_stty_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
