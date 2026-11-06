/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sensor_i2c.h"
static int signed12(unsigned v) {
    return (v&2048u)?(int)v-4096:(int)v;
}
int main(void) {
    int fd=i2c_open();
    if(fd<0) {
        puts("BMX055 acc: I2C open failed");
        return 1;
    }
    if(i2c_write_reg(fd,0x19,0x0f,3)<0||i2c_write_reg(fd,0x19,0x10,8)<0||i2c_write_reg(fd,0x19,0x11,0)<0) {
        puts("acc: init failed");
        close(fd);
        return 1;
    }
    for(;;) {
        uint8_t b[6];
        msleep(100);
        if(i2c_read_reg(fd,0x19,0x02,b,6)<0) {
            puts("acc: read failed");
            break;
        }
        unsigned x=(u16le(b)&0xfff0)>>4,y=(u16le(b+2)&0xfff0)>>4,z=(u16le(b+4)&0xfff0)>>4;
        printf("%d %d %d (raw, +/-2g)\n",signed12(x),signed12(y),signed12(z));
    }
    close(fd);
    return 1;
}
