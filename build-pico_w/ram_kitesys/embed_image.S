/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.kitesys,"a",%progbits
.balign 4
.global piconix_kitesys_image_start
.global piconix_kitesys_image_end
piconix_kitesys_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_kitesys/kitesys.bin"
piconix_kitesys_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
