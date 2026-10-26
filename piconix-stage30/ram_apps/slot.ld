/* GNU ld / lld compatible standalone RP2040 task image. */
ENTRY(app_entry)
SLOT_ORIGIN = @APP_ORIGIN@;
/* Entrypoint is validated from the ELF by check_slot_image.py. */
SLOT_SIZE = 0x10000;
STACK_BYTES = 0x1000;
MEMORY { TASK (rwx) : ORIGIN = SLOT_ORIGIN, LENGTH = SLOT_SIZE }
SECTIONS {
  . = ORIGIN(TASK);
  .text : ALIGN(4) {
    KEEP(*(.text.app_entry))
    *(.text .text.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
  } > TASK
  .data : ALIGN(4) { *(.data .data.*) . = ALIGN(4); } > TASK
  __image_end = .;
  .bss (NOLOAD) : ALIGN(4) {
    __bss_start = .;
    *(.bss .bss.* COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > TASK
  __stack_bottom = ORIGIN(TASK) + LENGTH(TASK) - STACK_BYTES;
  __stack_top = ORIGIN(TASK) + LENGTH(TASK);
  ASSERT(__bss_end <= __stack_bottom, "task image overlaps reserved stack")
  /DISCARD/ : { *(.comment) *(.note*) *(.ARM.attributes) *(.eh_frame*) }
}
