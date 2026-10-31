/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.edit,"a",%progbits
.balign 4
.global piconix_edit_image_start
.global piconix_edit_image_end
piconix_edit_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_edit/edit.bin"
piconix_edit_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
