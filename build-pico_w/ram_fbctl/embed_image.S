/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.fbctl,"a",%progbits
.balign 4
.global piconix_fbctl_image_start
.global piconix_fbctl_image_end
piconix_fbctl_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_fbctl/fbctl.bin"
piconix_fbctl_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
