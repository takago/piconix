/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.pwd,"a",%progbits
.balign 4
.global piconix_pwd_image_start
.global piconix_pwd_image_end
piconix_pwd_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_pwd/pwd.bin"
piconix_pwd_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
