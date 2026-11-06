/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Bosch BME280 integer compensation (temperature, pressure, humidity). */
#include "sensor_i2c.h"
static uint8_t c[32];
static int32_t tfine;
static int16_t digt2,digt3,digp[9],digh2,digh4,digh5;
static uint16_t digt1,digp1;
static uint8_t digh1,digh3;
static int8_t digh6;
static int32_t temp_comp(int32_t adc) {
    int32_t v1=((((adc>>3)-((int32_t)digt1<<1)))*digt2)>>11;
    int32_t v2=(((((adc>>4)-(int32_t)digt1)*((adc>>4)-(int32_t)digt1))>>12)*digt3)>>14;
    tfine=v1+v2;
    return (tfine*5+128)>>8;
    /* centi-degrees */
}
static uint32_t press_comp(int32_t adc) {
    int64_t v1=(int64_t)tfine-128000,v2=v1*v1*digp[5];
    v2+=((v1*digp[4])<<17);
    v2+=((int64_t)digp[3]<<35);
    v1=((v1*v1*digp[2])>>8)+((v1*digp[1])<<12);
    v1=((((int64_t)1<<47)+v1)*digp1)>>33;
    if(!v1)return 0;
    int64_t p=1048576-adc;
    p=(((p<<31)-v2)*3125)/v1;
    v1=(digp[8]*(p>>13)*(p>>13))>>25;
    v2=(digp[7]*p)>>19;
    p=((p+v1+v2)>>8)+((int64_t)digp[6]<<4);
    return p<0?0:(uint32_t)p;
    /* Q24.8 Pa */
}
static uint32_t hum_comp(int32_t adc) {
    int32_t v=tfine-76800;
    v=(((((adc<<14)-((int32_t)digh4<<20)-((int32_t)digh5*v))+16384)>>15)*     (((((((v*digh6)>>10)*(((v*digh3)>>11)+32768))>>10)+2097152)*digh2+8192)>>14));
    v=v-(((((v>>15)*(v>>15))>>7)*digh1)>>4);
    if(v<0)v=0;
    if(v>419430400)v=419430400;
    return (uint32_t)(v>>12);
    /* Q22.10 %RH */
}
int main(void) {
    int fd=i2c_open();
    if(fd<0) {
        puts("BME280: I2C open failed");
        return 1;
    }
    /* humidity control must precede ctrl_meas to latch oversampling. */
    if(i2c_write_reg(fd,0x76,0xf2,1)<0||i2c_write_reg(fd,0x76,0xf5,0xa0)<0||i2c_write_reg(fd,0x76,0xf4,0x27)<0) {
        puts("BME280: init failed");
        close(fd);
        return 1;
    }
    uint8_t a[26],b[7];
    if(i2c_read_reg(fd,0x76,0x88,a,26)<0||i2c_read_reg(fd,0x76,0xe1,b,7)<0) {
        puts("BME280: trim failed");
        close(fd);
        return 1;
    }
    digt1=u16le(a);
    digt2=s16(u16le(a+2));
    digt3=s16(u16le(a+4));
    digp1=u16le(a+6);
    for(int i=1;i<9;i++)digp[i]=s16(u16le(a+6+2*i));
    digh1=a[25];
    digh2=s16(u16le(b));
    digh3=b[2];
    digh4=(int16_t)((((int16_t)(int8_t)b[3])<<4)|(b[4]&15));
    digh5=(int16_t)((((int16_t)(int8_t)b[5])<<4)|(b[4]>>4));
    digh6=(int8_t)b[6];
    for(;;) {
        uint8_t r[8];
        msleep(1000);
        if(i2c_read_reg(fd,0x76,0xf7,r,8)<0) {
            puts("BME280: read failed");
            break;
        }
        int32_t rp=((int32_t)r[0]<<12)|((int32_t)r[1]<<4)|(r[2]>>4);
        int32_t rt=((int32_t)r[3]<<12)|((int32_t)r[4]<<4)|(r[5]>>4);
        int32_t rh=((int32_t)r[6]<<8)|r[7];
        int32_t t=temp_comp(rt);
        uint32_t pr=press_comp(rp),h=hum_comp(rh);
        int32_t ta=t<0?-t:t;
        printf("T=%s%ld.%02ld C P=%lu.%02lu hPa H=%lu.%01lu %%\n",   t<0?"-":"",(long)(ta/100),(long)(ta%100),   (unsigned long)(pr/25600),(unsigned long)(((pr%25600)*100u)/25600),   (unsigned long)(h/1024),(unsigned long)(((h%1024)*10u)/1024));
    }
    close(fd);
    return 1;
}
