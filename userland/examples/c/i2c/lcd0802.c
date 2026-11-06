/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "sensor_i2c.h"
static int lcd(int fd,int prefix,const uint8_t *data,int n) {
    uint8_t b[32];
    if(n>31)return -1;
    b[0]=(uint8_t)prefix;
    for(int i=0;i<n;i++)b[i+1]=data[i];
    return i2c_xfer(fd,0x3e,b,n+1,0,0);
}
int main(void) {
    int fd=i2c_open();
    if(fd<0) {
        puts("LCD0802: I2C open failed");
        return 1;
    }
    const uint8_t init[]= {
        0x38,0x39,0x14,0x70,0x56,0x6c
    };
    const uint8_t setup[]= {
        0x38,0x0d,0x01
    },
    row2[]= {
        0xc0
    };
    const uint8_t hello[]= {
        'H','e','l','l','o'
    },
    face[]= {
        '(','o','_','o',')'
    };
    int ok=lcd(fd,0,init,6);
    msleep(200);
    if(ok==0)ok=lcd(fd,0,setup,3);
    msleep(2);
    if(ok==0)ok=lcd(fd,0x40,hello,5);
    msleep(1);
    if(ok==0)ok=lcd(fd,0,row2,1);
    msleep(2);
    if(ok==0)ok=lcd(fd,0x40,face,5);
    close(fd);
    if(ok<0) {
        puts("LCD0802: transfer failed");
        return 1;
    }
    return 0;
}
