/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.lsof,"a",%progbits
.balign 4
.global piconix_lsof_image_start
.global piconix_lsof_image_end
piconix_lsof_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_lsof/lsof.bin"
piconix_lsof_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
