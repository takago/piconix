/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sensor_i2c.h"
int main(void) {
    int fd=i2c_open();
    if(fd<0) {
        puts("BH1750: I2C open failed");
        return 1;
    }
    uint8_t on=1,mode=0x10,off=0;
    if(i2c_xfer(fd,0x23,&on,1,0,0)<0||i2c_xfer(fd,0x23,&mode,1,0,0)<0) {
        puts("BH1750: init failed");
        close(fd);
        return 1;
    }
    for(;;) {
        uint8_t b[2];
        msleep(200);
        if(i2c_xfer(fd,0x23,0,0,b,2)<0) {
            puts("BH1750: read failed");
            break;
        }
        unsigned raw=((unsigned)b[0]<<8)|b[1];
        unsigned lx10=raw*25u/3u;
        printf("%u.%u lx\n",lx10/10,lx10%10);
    }
    (void)i2c_xfer(fd,0x23,&off,1,0,0);
    close(fd);
    return 1;
}
