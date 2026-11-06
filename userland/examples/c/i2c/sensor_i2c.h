/* PicoNIX I2C sample helpers. Each transfer uses /dev/i2c/1. */
#ifndef SENSOR_I2C_H
#define SENSOR_I2C_H
#include "unistd.h"
#include "fcntl.h"
#include "stdio.h"
#include <stdint.h>
#include "piconix_i2c.h"
static int i2c_open(void) {
    return open("/dev/i2c/1", O_RDWR);
}
static int i2c_xfer(int fd, int addr, const uint8_t *w, int nw, uint8_t *r, int nr) {
    piconix_i2c_transfer_t t= {
        0
    };
    if(nw<0 || nw>32 || nr<0 || nr>32)
        return -1;
    t.address=(uint8_t)addr;
    t.write_len=(uint8_t)nw;
    t.read_len=(uint8_t)nr;
    for(int i=0;i<nw;i++)
        t.write_data[i]=w[i];
    if(ioctl(fd,OS_I2C_TRANSFER,(int)(uintptr_t)&t)<0)
        return -1;
    for(int i=0;i<nr;i++)
        r[i]=t.read_data[i];
    return 0;
}
static int i2c_read_reg(int fd,int addr,int reg,uint8_t *buf,int n) {
    uint8_t b=(uint8_t)reg;
    return i2c_xfer(fd,addr,&b,1,buf,n);
}
static int i2c_write_reg(int fd,int addr,int reg,int value) {
    uint8_t b[2]= {
        (uint8_t)reg,(uint8_t)value
    };
    return i2c_xfer(fd,addr,b,2,0,0);
}
static unsigned u16le(const uint8_t *p) {
    return (unsigned)p[0]|((unsigned)p[1]<<8);
}
static int s16(unsigned v) {
    return (v&32768u)?(int)v-65536:(int)v;
}
#endif
