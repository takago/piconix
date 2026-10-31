/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.tcp_echod,"a",%progbits
.balign 4
.global piconix_tcp_echod_image_start
.global piconix_tcp_echod_image_end
piconix_tcp_echod_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_tcp_echod/tcp_echod.bin"
piconix_tcp_echod_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
