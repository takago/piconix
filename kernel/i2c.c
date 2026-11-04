/* SPDX-License-Identifier: GPL-3.0-or-later */
/* ユーザのI2C転送要求をハードウェアドライバへ中継する． */

#include "devices.h"
#include "../include/piconix_i2c.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
/* I2C1を初期化し，使用ピンをI2C機能へ設定する． */
void i2c_driver_init(void) {
    i2c_init(i2c1,100000);
    gpio_set_function(26,GPIO_FUNC_I2C);
    gpio_set_function(27,GPIO_FUNC_I2C);
    gpio_pull_up(26);
    gpio_pull_up(27);
}

/* アドレス・転送長を検査し，I2Cの書込みと読出しを一つの要求として実行する． */
int i2c_driver_transfer(piconix_i2c_transfer_t *t) {
    if(t->address<0x08 || t->address>0x77 || t->reserved ||
       t->write_len>PICONIX_I2C_MAX_TRANSFER ||
       t->read_len>PICONIX_I2C_MAX_TRANSFER) return -22;
    int r;
    if(!t->write_len && !t->read_len) {
        r=i2c_write_timeout_us(i2c1,t->address,0,0,false,PICONIX_I2C_TIMEOUT_US);
        return r<0 ? -5 : 0;
    }
    if(t->write_len) {
        r=i2c_write_timeout_us(i2c1,t->address,t->write_data,t->write_len,
                               t->read_len!=0,PICONIX_I2C_TIMEOUT_US);
        if(r!=t->write_len) return -5;
    }
    if(t->read_len) {
        r=i2c_read_timeout_us(i2c1,t->address,t->read_data,t->read_len,false,PICONIX_I2C_TIMEOUT_US);
        if(r!=t->read_len) return -5;
    }
    return 0;
}
