/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sensor_i2c.h"
int main(void) {
    int fd=i2c_open();
    if(fd<0) {
        puts("ADT7410: I2C open failed");
        return 1;
    }
    for(;;) {
        uint8_t b[2];
        if(i2c_read_reg(fd,0x48,0x00,b,2)<0) {
            puts("ADT7410: read failed");
            break;
        }
        int raw=s16(((unsigned)b[0]<<8)|b[1]);
        /* default 13-bit resolution */
        int c100=raw*100/128;
        int absval=c100<0?-c100:c100;
        printf("%s%d.%02d C\n",c100<0?"-":"",absval/100,absval%100);
        msleep(250);
    }
    close(fd);
    return 1;
}
