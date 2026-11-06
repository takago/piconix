/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sensor_i2c.h"
int main(void) {
    int fd=i2c_open();
    if(fd<0) {
        puts("BMX055 gyro: I2C open failed");
        return 1;
    }
    if(i2c_write_reg(fd,0x69,0x0f,0x10)<0||i2c_write_reg(fd,0x69,0x10,0x07)<0||i2c_write_reg(fd,0x69,0x11,0)<0) {
        puts("gyro: init failed");
        close(fd);
        return 1;
    }
    for(;;) {
        uint8_t b[6];
        msleep(100);
        if(i2c_read_reg(fd,0x69,0x02,b,6)<0) {
            puts("gyro: read failed");
            break;
        }
        printf("%d %d %d (raw)\n",s16(u16le(b)),s16(u16le(b+2)),s16(u16le(b+4)));
    }
    close(fd);
    return 1;
}
