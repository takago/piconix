/* SPDX-License-Identifier: GPL-3.0-or-later */
/* GPIO状態照会のビット表現を共有する． */

#ifndef PICONIX_GPIO_STATUS_H
#define PICONIX_GPIO_STATUS_H
/* ioctl(/dev/gpio/N, OS_GPIO_STATUS_BASE + pin, 0)はGP0〜28の実ハードウェア状態をまとめて返す．複製した状態表は持たない． */
#define OS_GPIO_STATUS_BASE 0x400
#define GPIO_STATUS_FREE 0
#define GPIO_STATUS_IN 1
#define GPIO_STATUS_OUT 2
#define GPIO_STATUS_FB 3
#define GPIO_STATUS_PWM 4
#define GPIO_STATUS_ADC 5
#define GPIO_STATUS_UART_TX 6
#define GPIO_STATUS_UART_RX 7
#define GPIO_STATUS_OTHER 8
#define GPIO_STATUS_I2C 9
#define GPIO_STATUS_RESERVED 10
#define GPIO_STATUS_MODE_MASK 15
#define GPIO_STATUS_LEVEL_SHIFT 4
#define GPIO_STATUS_PULL_SHIFT 5
#endif
