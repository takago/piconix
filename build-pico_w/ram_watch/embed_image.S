/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.watch,"a",%progbits
.balign 4
.global piconix_watch_image_start
.global piconix_watch_image_end
piconix_watch_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_watch/watch.bin"
piconix_watch_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
