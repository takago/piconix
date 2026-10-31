/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.httpd,"a",%progbits
.balign 4
.global piconix_httpd_image_start
.global piconix_httpd_image_end
piconix_httpd_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_httpd/httpd.bin"
piconix_httpd_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
