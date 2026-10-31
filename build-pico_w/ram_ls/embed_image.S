/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.ls,"a",%progbits
.balign 4
.global piconix_ls_image_start
.global piconix_ls_image_end
piconix_ls_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_ls/ls.bin"
piconix_ls_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
