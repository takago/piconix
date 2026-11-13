/* getdents()が返す固定長ディレクトリエントリの共通形式． */
/* PicoNIXのgetdents用固定長ディレクトリエントリ形式． */
#ifndef PICONIX_DIRENT_H
#define PICONIX_DIRENT_H
#include <stdint.h>
typedef struct {
    char name[32];       /* ディレクトリエントリの名前． */
    uint8_t is_dir;      /* ディレクトリであることを示すフラグ． */
    uint16_t device_id;  /* VFS内のデバイス・ノード識別子． */
} piconix_dirent_t;
#endif
