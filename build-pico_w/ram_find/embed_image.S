/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.find,"a",%progbits
.balign 4
.global piconix_find_image_start
.global piconix_find_image_end
piconix_find_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_find/find.bin"
piconix_find_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
