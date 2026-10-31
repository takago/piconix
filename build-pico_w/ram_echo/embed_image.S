/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.echo,"a",%progbits
.balign 4
.global piconix_echo_image_start
.global piconix_echo_image_end
piconix_echo_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_echo/echo.bin"
piconix_echo_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
