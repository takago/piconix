/* Standalone RAM image, retained in firmware Flash. */
.section .piconix_app_image.trace,"a",%progbits
.balign 4
.global piconix_trace_image_start
.global piconix_trace_image_end
piconix_trace_image_start:
.incbin "/tmp/PicoNIX/build-pico_w/ram_trace/trace.bin"
piconix_trace_image_end:
.balign 4
.section .note.GNU-stack,"",%progbits
