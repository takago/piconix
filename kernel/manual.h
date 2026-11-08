/* PicoNIX /man manual stored in the dedicated XIP Flash .manual section.
 * Image: 8-byte magic, uint32 count, followed by fixed-size index and text.
 * The linker emits the start/end symbols; no manual contents enter SRAM.
 */
#ifndef PICONIX_MANUAL_H
#define PICONIX_MANUAL_H

#include <stdint.h>

typedef struct {
    char name[32];
    uint32_t offset; /* Byte offset from __manual_start. */
    uint32_t size;
} man_entry_t;

extern const unsigned char __manual_start[], __manual_end[];
#define MAN_BASE 500
#define MAN_COUNT (*(const uint32_t *)(__manual_start + 8))
#define man_entries ((const man_entry_t *)(const void *)(__manual_start + 12))
#define man_data(entry) (__manual_start + (entry)->offset)

#endif
