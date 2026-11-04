/* getdents()が返す固定長ディレクトリエントリの共通形式． */
/* PicoNIXのgetdents用固定長ディレクトリエントリ形式． */
#ifndef PICONIX_DIRENT_H
#define PICONIX_DIRENT_H
#include <stdint.h>
typedef struct { char name[32]; uint8_t is_dir; uint16_t device_id; } piconix_dirent_t;
#endif
