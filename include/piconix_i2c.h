/* SPDX-License-Identifier: GPL-3.0-or-later */
/* I2C ioctlの要求形式と定数をカーネル・ユーザ間で共有する． */

#ifndef PICONIX_I2C_H
#define PICONIX_I2C_H
#include <stdint.h>
/* ioctlで行うI2C転送．writeとreadが両方非ゼロならrepeated STARTで連結する．バッファはユーザRAMに置き，カーネルポインタを公開しない． */
#define OS_I2C_TRANSFER 0x520
#define PICONIX_I2C_MAX_TRANSFER 32
#define PICONIX_I2C_TIMEOUT_US 10000
/* 転送長0はアドレス応答の確認に使う． */
typedef struct {
    uint8_t address; /* 7ビットアドレス．許可範囲は0x08〜0x77． */
    uint8_t write_len;
    uint8_t read_len;
    uint8_t reserved;
    uint8_t write_data[PICONIX_I2C_MAX_TRANSFER];
    uint8_t read_data[PICONIX_I2C_MAX_TRANSFER];
} piconix_i2c_transfer_t;
#endif
