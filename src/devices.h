/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef PICONIX_DEVICES_H
#define PICONIX_DEVICES_H
#include <stddef.h>
void gpio_driver_init(void);
int gpio_driver_rw(int kind, unsigned pin, void *p, size_t n, int writing);
int gpio_driver_ioctl(unsigned pin, int cmd);
void adc_driver_init(void);
unsigned adc_driver_read(unsigned channel);
int adc_driver_format(unsigned raw, int temperature, char *text, size_t size);
void pwm_driver_init(void);
unsigned pwm_driver_get(unsigned which);
int pwm_driver_set(unsigned which, unsigned value);
#endif
