/* PicoNIX binary trace ABI: 16-byte event records. */
#ifndef PICONIX_TRACE_H
#define PICONIX_TRACE_H
#include <stdint.h>
#define TRACE_ON  1
#define TRACE_OFF 2
#define TRACE_SWITCH 1
#define TRACE_SVC_ENTER 2
#define TRACE_SVC_EXIT 3
typedef struct {
    uint32_t tick;
    uint32_t value;
    uint32_t detail;
    uint8_t kind;
    uint8_t pid;
    uint16_t reserved;
} trace_event_t;
#endif
