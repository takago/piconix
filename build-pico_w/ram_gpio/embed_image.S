/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.gpio,"a",%progbits
.balign 4
.global piconix_gpio_image_start
.global piconix_gpio_image_end
piconix_gpio_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_gpio/gpio.bin"
piconix_gpio_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
