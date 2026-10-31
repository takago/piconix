/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.mb,"a",%progbits
.balign 4
.global piconix_mb_image_start
.global piconix_mb_image_end
piconix_mb_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_mb/mb.bin"
piconix_mb_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
