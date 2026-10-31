/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.tree,"a",%progbits
.balign 4
.global piconix_tree_image_start
.global piconix_tree_image_end
piconix_tree_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_tree/tree.bin"
piconix_tree_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
