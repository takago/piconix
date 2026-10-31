/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.shell,"a",%progbits
.balign 4
.global piconix_shell_image_start
.global piconix_shell_image_end
piconix_shell_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_shell/shell.bin"
piconix_shell_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
