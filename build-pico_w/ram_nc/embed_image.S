/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.nc,"a",%progbits
.balign 4
.global piconix_nc_image_start
.global piconix_nc_image_end
piconix_nc_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_nc/nc.bin"
piconix_nc_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
