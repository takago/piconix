/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.pwmctl,"a",%progbits
.balign 4
.global piconix_pwmctl_image_start
.global piconix_pwmctl_image_end
piconix_pwmctl_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_pwmctl/pwmctl.bin"
piconix_pwmctl_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
