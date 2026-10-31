/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.pins,"a",%progbits
.balign 4
.global piconix_pins_image_start
.global piconix_pins_image_end
piconix_pins_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_pins/pins.bin"
piconix_pins_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
