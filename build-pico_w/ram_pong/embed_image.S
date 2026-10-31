/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.pong,"a",%progbits
.balign 4
.global piconix_pong_image_start
.global piconix_pong_image_end
piconix_pong_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_pong/pong.bin"
piconix_pong_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
