/* 16 FD用の小型select要求．SVC中のユーザメモリに置く． */
#ifndef PICONIX_SELECT_H
#define PICONIX_SELECT_H
#include <stdint.h>
#define PICONIX_FD_SETSIZE 16
/* 入力マスクは待機中も保存し，結果だけを別フィールドに返す． */
typedef struct {
    uint16_t read_mask;   /* 読取りを監視するFDビット集合． */
    uint16_t write_mask;  /* 書込みを監視するFDビット集合． */
    uint16_t ready_read;  /* 読取り準備が整ったFDのビット集合． */
    uint16_t ready_write; /* 書込み準備が整ったFDのビット集合． */
    int32_t timeout_ms;   /* -1: 無期限，0: 即時，正値: 最大待機ms． */
} piconix_select_request_t;
#endif
