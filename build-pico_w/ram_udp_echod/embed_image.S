/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.udp_echod,"a",%progbits
.balign 4
.global piconix_udp_echod_image_start
.global piconix_udp_echod_image_end
piconix_udp_echod_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_udp_echod/udp_echod.bin"
piconix_udp_echod_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
