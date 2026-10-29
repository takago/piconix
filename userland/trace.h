/* PicoNIX binary trace ABI: fixed 32-byte records. */
#ifndef PICONIX_TRACE_H
#define PICONIX_TRACE_H
#include <stdint.h>
#define TRACE_ON  1
#define TRACE_OFF 2
#define TRACE_STATUS 3
#define TRACE_LOST 4
#define TRACE_SWITCH 1
#define TRACE_SVC_ENTER 2
#define TRACE_SVC_EXIT 3
#define TRACE_TICK1K 4
typedef struct {
    uint32_t tick;
    uint32_t value;
    uint32_t detail;
    uint8_t kind;
    uint8_t pid;
    uint8_t program_id;  /* low 7 bits: program; high bit: uart1 TTY */
    int8_t file_id;      /* -1: no file / unknown */
    uint8_t data[16];    /* raw preview; not necessarily NUL-terminated */
} trace_event_t;
#endif
