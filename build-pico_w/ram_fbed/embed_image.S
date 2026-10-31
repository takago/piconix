/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.fbed,"a",%progbits
.balign 4
.global piconix_fbed_image_start
.global piconix_fbed_image_end
piconix_fbed_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_fbed/fbed.bin"
piconix_fbed_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
