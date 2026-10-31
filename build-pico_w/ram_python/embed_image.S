/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.python,"a",%progbits
.balign 4
.global piconix_python_image_start
.global piconix_python_image_end
piconix_python_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_python/python.bin"
piconix_python_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
