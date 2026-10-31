/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.hello,"a",%progbits
.balign 4
.global piconix_hello_image_start
.global piconix_hello_image_end
piconix_hello_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_hello/hello.bin"
piconix_hello_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
