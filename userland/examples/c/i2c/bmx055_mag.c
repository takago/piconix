/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sensor_i2c.h"
static int signed_bits(unsigned v,int bits) {
    unsigned sign=1u<<(bits-1);
    return (int)((v^sign)-sign);
}
int main(void) {
    int fd=i2c_open();
    if(fd<0) {
        puts("BMX055 mag: I2C open failed");
        return 1;
    }
    if(i2c_write_reg(fd,0x13,0x4b,1)<0) {
        puts("mag: wake failed");
        close(fd);
        return 1;
    }
    msleep(400);
    if(i2c_write_reg(fd,0x13,0x4c,0)<0||i2c_write_reg(fd,0x13,0x4e,0x84)<0||i2c_write_reg(fd,0x13,0x51,9)<0||i2c_write_reg(fd,0x13,0x52,15)<0) {
        puts("mag: init failed");
        close(fd);
        return 1;
    }
    for(;;) {
        uint8_t b[6];
        msleep(100);
        if(i2c_read_reg(fd,0x13,0x42,b,6)<0) {
            puts("mag: read failed");
            break;
        }
        unsigned x=(((unsigned)b[1]<<8)|(b[0]&0xf8))>>3;
        unsigned y=(((unsigned)b[3]<<8)|(b[2]&0xf8))>>3;
        unsigned z=(((unsigned)b[5]<<8)|(b[4]&0xfe))>>1;
        printf("%d %d %d (raw)\n",signed_bits(x,13),signed_bits(y,13),signed_bits(z,15));
    }
    close(fd);
    return 1;
}
