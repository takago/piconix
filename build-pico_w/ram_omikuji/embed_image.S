/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.omikuji,"a",%progbits
.balign 4
.global piconix_omikuji_image_start
.global piconix_omikuji_image_end
piconix_omikuji_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_omikuji/omikuji.bin"
piconix_omikuji_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
