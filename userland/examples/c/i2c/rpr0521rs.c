/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sensor_i2c.h"
int main(void) {
    int fd=i2c_open();
    if(fd<0) {
        puts("RPR0521RS: I2C open failed");
        return 1;
    }
    if(i2c_write_reg(fd,0x38,0x40,0x80)<0||i2c_write_reg(fd,0x38,0x41,0xc6)<0) {
        puts("RPR0521RS: init failed");
        close(fd);
        return 1;
    }
    for(;;) {
        uint8_t b[6];
        msleep(250);
        if(i2c_read_reg(fd,0x38,0x44,b,6)<0) {
            puts("RPR0521RS: read failed");
            break;
        }
        unsigned ps=((unsigned)(b[1]&15)<<8)|b[0];
        printf("PS=%u ALS0=%u ALS1=%u (raw)\n",ps,u16le(b+2),u16le(b+4));
    }
    close(fd);
    return 1;
}
