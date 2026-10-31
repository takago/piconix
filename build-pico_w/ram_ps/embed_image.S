/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.ps,"a",%progbits
.balign 4
.global piconix_ps_image_start
.global piconix_ps_image_end
piconix_ps_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_ps/ps.bin"
piconix_ps_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
