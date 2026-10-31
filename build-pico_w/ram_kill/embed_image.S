/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.kill,"a",%progbits
.balign 4
.global piconix_kill_image_start
.global piconix_kill_image_end
piconix_kill_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_kill/kill.bin"
piconix_kill_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
