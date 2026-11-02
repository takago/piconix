/* PicoNIX fixed-size directory entry ABI. */
#ifndef PICONIX_DIRENT_H
#define PICONIX_DIRENT_H
#include <stdint.h>
typedef struct { char name[32]; uint8_t is_dir; uint16_t device_id; } piconix_dirent_t;
#endif
