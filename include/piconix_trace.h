/* カーネルが記録し，ユーザのtraceが読み出すイベント形式． */
/* 固定32バイトのバイナリトレースABI． */
#ifndef PICONIX_TRACE_H
#define PICONIX_TRACE_H
#include <stdint.h>
#define TRACE_KIND_MASK 0x3fu
#define TRACE_TTY(kind) ((kind) >> 6)
#define TRACE_ON  1
#define TRACE_OFF 2
#define TRACE_STATUS 3
#define TRACE_LOST 4
#define TRACE_CONTEXT_SET 5 /* 0: SVC only, 1: all contexts */
#define TRACE_CONTEXT_GET 6
#define TRACE_SWITCH 1
#define TRACE_SVC_ENTER 2
#define TRACE_SVC_EXIT 3
#define TRACE_TICK1K 4
#define TRACE_FUNC_ENTER 5
#define TRACE_FUNC_EXIT 6
typedef struct {
    uint32_t tick;
    uint32_t value;
    uint32_t detail;
    uint8_t kind;  /* 下位6ビットにイベント，上位2ビットにTTY番号を格納する． */
    uint8_t pid;
    uint8_t program_id;  /* プログラムIDは省略せず記録する． */
    int8_t file_id;      /* -1は対象FDなし，または不明を表す． */
    uint8_t data[16];    /* 生データの先頭を記録する．NUL終端は保証されない． */
} trace_event_t;
#endif
