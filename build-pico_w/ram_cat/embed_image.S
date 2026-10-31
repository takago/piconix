/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.cat,"a",%progbits
.balign 4
.global piconix_cat_image_start
.global piconix_cat_image_end
piconix_cat_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_cat/cat.bin"
piconix_cat_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
