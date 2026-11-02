/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef PICONIX_I2C_H
#define PICONIX_I2C_H
#include <stdint.h>
/* ioctl(fd, OS_I2C_TRANSFER, (int)(uintptr_t)&request).
 * Atomic write-then-read transaction with repeated START when both lengths
 * are nonzero.  Buffers are in user RAM; no raw kernel pointers are exposed.
 */
#define OS_I2C_TRANSFER 0x520
#define PICONIX_I2C_MAX_TRANSFER 32
#define PICONIX_I2C_TIMEOUT_US 10000
/* A zero-length transfer probes the address. */
typedef struct {
    uint8_t address; /* 7-bit address, 0x08..0x77 */
    uint8_t write_len;
    uint8_t read_len;
    uint8_t reserved;
    uint8_t write_data[PICONIX_I2C_MAX_TRANSFER];
    uint8_t read_data[PICONIX_I2C_MAX_TRANSFER];
} piconix_i2c_transfer_t;
#endif
