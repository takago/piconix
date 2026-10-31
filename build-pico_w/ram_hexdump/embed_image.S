/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.hexdump,"a",%progbits
.balign 4
.global piconix_hexdump_image_start
.global piconix_hexdump_image_end
piconix_hexdump_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_hexdump/hexdump.bin"
piconix_hexdump_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
