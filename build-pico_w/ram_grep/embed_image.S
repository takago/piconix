/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.grep,"a",%progbits
.balign 4
.global piconix_grep_image_start
.global piconix_grep_image_end
piconix_grep_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_grep/grep.bin"
piconix_grep_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
